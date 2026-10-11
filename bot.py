import asyncio
import sqlite3
import os
import logging
from dotenv import load_dotenv
from datetime import datetime, timedelta
from aiogram import Bot, Dispatcher, F, types
from aiogram.filters import Command
from aiogram.fsm.context import FSMContext
from aiogram.fsm.state import State, StatesGroup
from aiogram.types import Message, LabeledPrice, PreCheckoutQuery, ReplyKeyboardRemove, FSInputFile
from werkzeug.security import check_password_hash

# Включаем логирование
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(name)s - %(message)s",
)
logger = logging.getLogger(__name__)
load_dotenv()

# --- НАСТРОЙКИ ---
BOT_TOKEN = os.getenv("BOT_TOKEN")  # <--- Вставь свой токен от @BotFather

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(BASE_DIR, "data")
SCHEMS_DIR = os.path.join(BASE_DIR, "Schems")

os.makedirs(DATA_DIR, exist_ok=True)
DB_PATH = os.path.join(DATA_DIR, "app.db")
# -----------------

bot = Bot(token=BOT_TOKEN, is_test=False) # Поставь is_test=False, когда перейдешь на реальные звезды
dp = Dispatcher()


class AuthStates(StatesGroup):
    waiting_for_login = State()
    waiting_for_password = State()


# 0. Команда /start — отправка файла с правилами из папки Schems
@dp.message(Command("start"))
async def cmd_start(message: Message):
    docs_path = os.path.join(SCHEMS_DIR, "docs.html")
    
    caption_text = (
        "📜 **Ознакомьтесь с правилами сервиса.**\n\n"
        "Если вы согласны с условиями использования и политикой возврата, продолжите вход, "
        "написав команду: /login"
    )

    if os.path.exists(docs_path):
        document = FSInputFile(docs_path)
        await message.answer_document(
            document=document,
            caption=caption_text
        )
    else:
        # Если файл случайно забыли положить, пришлем текст, чтобы бот не пал
        await message.answer(
            "⚠️ Файл с правилами не найден на сервере, но вы можете продолжить.\n\n" + caption_text
        )


# 1. Авторизация: шаг 1 (/login)
@dp.message(Command("login"))
async def cmd_login(message: Message, state: FSMContext):
    await message.answer(
        "🔐 **Авторизация в системе**\n\nВведите ваш **логин** или email от сайта:",
        reply_markup=ReplyKeyboardRemove(),
    )
    await state.set_state(AuthStates.waiting_for_login)


@dp.message(AuthStates.waiting_for_login)
async def process_login(message: Message, state: FSMContext):
    await state.update_data(login=message.text)
    await message.answer("🔑 Введите ваш **пароль**:")
    await state.set_state(AuthStates.waiting_for_password)


@dp.message(AuthStates.waiting_for_password)
async def process_password(message: Message, state: FSMContext):
    if message.text.startswith("/"):
        await message.answer("⚠️ Пожалуйста, завершите авторизацию, введя пароль.")
        return

    password = message.text
    data = await state.get_data()
    login = data.get("login")

    try:
        conn = sqlite3.connect(DB_PATH)
        cursor = conn.cursor()
        
        cursor.execute(
            "SELECT id, username, password_hash FROM users WHERE email = ? OR username = ?",
            (login, login),
        )
        user = cursor.fetchone()
        conn.close()

        if user:
            user_id_db, username_db, password_hash = user
            display_name = username_db if username_db else "Пользователь"

            if check_password_hash(password_hash, password) or password == password_hash:
                await state.clear()
                await state.update_data(user_id_db=user_id_db)
                
                await message.answer(
                    f"✅ **Успешный вход!**\nПривет, {display_name} (ID: {user_id_db}).\n\n"
                    f"Теперь вы можете купить подписку за 15 звёзд, написав команду: /buy"
                )
            else:
                await message.answer("❌ Неверный пароль. Попробуйте снова через /login")
                await state.clear()
        else:
            await message.answer("❌ Пользователь не найден в базе. Попробуйте снова через /login")
            await state.clear()

    except Exception as e:
        logger.error(f"Ошибка при авторизации: {e}")
        await message.answer(f"⚠️ Ошибка базы данных: {e}")
        await state.clear()


# 2. Команда /buy — выставление счета на 15 звёзд
@dp.message(Command("buy"))
async def cmd_buy(message: Message, state: FSMContext):
    data = await state.get_data()
    user_id_db = data.get("user_id_db")

    if not user_id_db:
        await message.answer("⚠️ Сначала нужно авторизоваться! Напишите команду: /login")
        return

    # Стоимость подписки изменена на 15 звёзд
    prices = [LabeledPrice(label="PRO Подписка (30 дней)", amount=1)]

    await message.answer_invoice(
        title="Оплата PRO-тарифа",
        description="Подписка на сайт сроком на 30 дней.",
        payload=f"sub_user_{user_id_db}",
        currency="XTR",
        prices=prices,
        provider_token="",
    )


@dp.pre_checkout_query()
async def process_pre_checkout(pre_checkout_query: PreCheckoutQuery):
    await pre_checkout_query.answer(ok=True)


# 3. Успешная оплата (автовыдача на 30 дней, статус 'paid')
@dp.message(F.successful_payment)
async def process_successful_payment(message: Message):
    payment_info = message.successful_payment
    payload = payment_info.invoice_payload
    stars_amount = payment_info.total_amount

    try:
        user_id_db = int(payload.split("_")[-1])
    except Exception as e:
        logger.error(f"Не удалось распарсить payload ({payload}): {e}")
        await message.answer("⚠️ Ошибка: не удалось определить ID пользователя.")
        return

    try:
        conn = sqlite3.connect(DB_PATH)
        cursor = conn.cursor()

        days = 30
        now_str = datetime.utcnow().isoformat(timespec="seconds")

        # Записываем платеж со статусом 'paid'
        cursor.execute(
            "INSERT INTO payments (user_id, amount, currency, days, status, created, paid_at) VALUES (?, ?, ?, ?, ?, ?, ?)",
            (user_id_db, str(stars_amount), "XTR", days, "paid", now_str, now_str),
        )

        # Продлеваем подписку в таблице users
        u = cursor.execute("SELECT subscription_until FROM users WHERE id=?", (user_id_db,)).fetchone()
        base = (
            datetime.fromisoformat(u[0])
            if u and u[0] and datetime.fromisoformat(u[0]) > datetime.utcnow()
            else datetime.utcnow()
        )
        new_sub_until = (base + timedelta(days=days)).isoformat()

        cursor.execute("UPDATE users SET subscription_until=? WHERE id=?", (new_sub_until, user_id_db))

        conn.commit()
        conn.close()

        await message.answer(
            f"🎉 **Оплата прошла успешно!**\n\n"
            f"• Списано звёзд: {stars_amount} ⭐\n"
            f"• Статус в базе `payments`: `paid`\n"
            f"• Подписка продлена до: `{new_sub_until[:10]}`\n\n"
            f"Обновите страницу сайта — PRO-статус уже активен!"
        )
    except Exception as e:
        logger.error(f"Ошибка при сохранении платежа: {e}")
        await message.answer(f"⚠️ Оплата прошла, но произошла ошибка при обновлении базы данных: {e}")


# 4. Команда /stop_subscription для отмены подписки (без возврата средств)
@dp.message(Command("stop_subscription"))
async def cmd_stop_subscription(message: Message, state: FSMContext):
    data = await state.get_data()
    user_id_db = data.get("user_id_db")

    if not user_id_db:
        await message.answer("⚠️ Сначала нужно авторизоваться! Напишите команду: /login")
        return

    try:
        conn = sqlite3.connect(DB_PATH)
        cursor = conn.cursor()

        cursor.execute("SELECT subscription_until FROM users WHERE id=?", (user_id_db,))
        user = cursor.fetchone()

        if not user or not user[0]:
            await message.answer("ℹ️ У вас нет активной подписки.")
            conn.close()
            return

        now_str = (datetime.utcnow() - timedelta(seconds=1)).isoformat()
        cursor.execute("UPDATE users SET subscription_until=? WHERE id=?", (now_str, user_id_db))
        
        conn.commit()
        conn.close()

        await message.answer(
            "🛑 **Подписка успешно отменена.**\n\n"
            "Доступ к платным материалам на сайте приостановлен.\n"
            "⚠️ *Обратите внимание: согласно правилам сервиса, уплаченные средства возврату не подлежат.*"
        )
    except Exception as e:
        logger.error(f"Ошибка при отмене подписки: {e}")
        await message.answer(f"⚠️ Произошла ошибка при обновлении базы данных: {e}")


async def main():
    if os.path.exists(DB_PATH):
        logger.info(f"База данных успешно найдена: {DB_PATH}")
    else:
        logger.error(f"Файл базы данных НЕ НАЙДЕН по пути: {DB_PATH}")

    logger.info("Бот запущен и готов к работе...")
    await dp.start_polling(bot)


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except (KeyboardInterrupt, SystemExit):
        logger.info("Бот остановлен.")