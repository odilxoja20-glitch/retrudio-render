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
from werkzeug.security import check_password_hash, generate_password_hash

# Включаем логирование
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(name)s - %(message)s",
)
logger = logging.getLogger(__name__)
load_dotenv()

# --- НАСТРОЙКИ ---
BOT_TOKEN = os.getenv("BOT_TOKEN")
SUBSCRIPTION_PRICE = os.getenv("SUBSCRIPTION_PRICE")
SITE_URL = os.getenv("SITE_URL", "https://retrudio.com")

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(BASE_DIR, "data")
SCHEMS_DIR = os.path.join(BASE_DIR, "Schems")

os.makedirs(DATA_DIR, exist_ok=True)
DB_PATH = os.path.join(DATA_DIR, "app.db")

# Защита от брутфорса: {telegram_id: {"attempts": int, "blocked_until": datetime}}
_login_attempts: dict = {}
MAX_ATTEMPTS = 5          # максимум попыток
BLOCK_DURATION_MIN = 15   # блокировка на 15 минут
# -----------------

bot = Bot(token=BOT_TOKEN, is_test=False)
dp = Dispatcher()


class AuthStates(StatesGroup):
    waiting_for_login = State()
    waiting_for_password = State()


class ChangePasswordStates(StatesGroup):
    waiting_for_old_password = State()
    waiting_for_new_password = State()
    waiting_for_confirm_password = State()


# --- Вспомогательные функции ---

def is_blocked(tg_id: int) -> tuple[bool, int]:
    """Проверяет, заблокирован ли пользователь. Возвращает (заблокирован, минут_осталось)."""
    info = _login_attempts.get(tg_id)
    if not info:
        return False, 0
    if info.get("blocked_until") and datetime.utcnow() < info["blocked_until"]:
        remaining = (info["blocked_until"] - datetime.utcnow()).seconds // 60 + 1
        return True, remaining
    return False, 0


def record_failed_attempt(tg_id: int) -> int:
    """Записывает неудачную попытку. Возвращает количество оставшихся попыток."""
    if tg_id not in _login_attempts:
        _login_attempts[tg_id] = {"attempts": 0, "blocked_until": None}

    _login_attempts[tg_id]["attempts"] += 1
    attempts = _login_attempts[tg_id]["attempts"]

    if attempts >= MAX_ATTEMPTS:
        _login_attempts[tg_id]["blocked_until"] = datetime.utcnow() + timedelta(minutes=BLOCK_DURATION_MIN)
        _login_attempts[tg_id]["attempts"] = 0
        return 0

    return MAX_ATTEMPTS - attempts


def reset_attempts(tg_id: int):
    """Сбрасывает счётчик попыток после успешного входа."""
    if tg_id in _login_attempts:
        del _login_attempts[tg_id]


# --- Команды ---

# /start
@dp.message(Command("start"))
async def cmd_start(message: Message):
    docs_path = os.path.join(SCHEMS_DIR, "docs.html")
    caption_text = (
        "📜 **Ознакомьтесь с правилами сервиса.**\n\n"
        "Если вы согласны с условиями использования и политикой возврата, "
        "войдите в аккаунт командой: /login\n\n"
        "📋 Список команд: /help"
    )
    if os.path.exists(docs_path):
        document = FSInputFile(docs_path)
        await message.answer_document(document=document, caption=caption_text)
    else:
        await message.answer(
            "⚠️ Файл с правилами не найден, но вы можете продолжить.\n\n" + caption_text
        )


# /help
@dp.message(Command("help"))
async def cmd_help(message: Message):
    await message.answer(
        "📋 **Список команд:**\n\n"
        "/start — начало работы и правила\n"
        "/login — войти в аккаунт\n"
        "/buy — купить подписку (30 дней)\n"
        "/status — проверить статус подписки\n"
        "/change_password — сменить пароль\n"
        "/stop_subscription — отменить подписку\n"
        "/help — это сообщение"
    )


# /status
@dp.message(Command("status"))
async def cmd_status(message: Message, state: FSMContext):
    data = await state.get_data()
    user_id_db = data.get("user_id_db")

    if not user_id_db:
        await message.answer("⚠️ Сначала войдите в аккаунт: /login")
        return

    try:
        conn = sqlite3.connect(DB_PATH)
        cursor = conn.cursor()
        cursor.execute("SELECT username, email, subscription_until FROM users WHERE id=?", (user_id_db,))
        user = cursor.fetchone()
        conn.close()

        if not user:
            await message.answer("❌ Пользователь не найден.")
            return

        username, email, sub_until = user
        now = datetime.utcnow()

        if sub_until:
            sub_dt = datetime.fromisoformat(sub_until)
            if sub_dt > now:
                days_left = (sub_dt - now).days
                status_text = f"✅ **Активна** до `{sub_until[:10]}` (осталось {days_left} дн.)"
            else:
                status_text = "❌ **Истекла**"
        else:
            status_text = "❌ **Нет подписки**"

        await message.answer(
            f"👤 **Аккаунт:**\n"
            f"• Имя: `{username or '—'}`\n"
            f"• Email: `{email or '—'}`\n\n"
            f"⭐ **Подписка:** {status_text}\n\n"
            f"Купить/продлить: /buy"
        )
    except Exception as e:
        logger.error(f"Ошибка /status: {e}")
        await message.answer(f"⚠️ Ошибка: {e}")


# /login — шаг 1
@dp.message(Command("login"))
async def cmd_login(message: Message, state: FSMContext):
    tg_id = message.from_user.id
    blocked, mins = is_blocked(tg_id)
    if blocked:
        await message.answer(
            f"🚫 **Слишком много неудачных попыток.**\n"
            f"Повторите через {mins} мин."
        )
        return

    await message.answer(
        "🔐 **Авторизация**\n\nВведите ваш **логин** или email от сайта:",
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
    tg_id = message.from_user.id

    if message.text.startswith("/"):
        await message.answer("⚠️ Завершите авторизацию — введите пароль.")
        return

    # Удаляем сообщение с паролем
    try:
        await message.delete()
    except Exception:
        pass

    blocked, mins = is_blocked(tg_id)
    if blocked:
        await state.clear()
        await message.answer(
            f"🚫 **Слишком много неудачных попыток.**\n"
            f"Повторите через {mins} мин."
        )
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
                reset_attempts(tg_id)
                await state.clear()
                await state.update_data(user_id_db=user_id_db)
                await message.answer(
                    f"✅ **Вход выполнен!**\n"
                    f"Привет, {display_name}!\n\n"
                    f"• Статус подписки: /status\n"
                    f"• Купить подписку: /buy\n"
                    f"• Сменить пароль: /change\\_password"
                )
            else:
                remaining = record_failed_attempt(tg_id)
                if remaining == 0:
                    await state.clear()
                    await message.answer(
                        f"🚫 **Превышен лимит попыток.**\n"
                        f"Аккаунт временно заблокирован на {BLOCK_DURATION_MIN} мин."
                    )
                else:
                    await message.answer(
                        f"❌ Неверный пароль.\n"
                        f"Осталось попыток: {remaining}\n"
                        f"Попробуйте снова: /login"
                    )
                    await state.clear()
        else:
            record_failed_attempt(tg_id)
            await message.answer("❌ Пользователь не найден. Попробуйте снова: /login")
            await state.clear()

    except Exception as e:
        logger.error(f"Ошибка авторизации: {e}")
        await message.answer(f"⚠️ Ошибка базы данных: {e}")
        await state.clear()


# /buy
@dp.message(Command("buy"))
async def cmd_buy(message: Message, state: FSMContext):
    data = await state.get_data()
    user_id_db = data.get("user_id_db")

    if not user_id_db:
        await message.answer("⚠️ Сначала войдите в аккаунт: /login")
        return

    prices = [LabeledPrice(label="Подписка (30 дней)", amount=int(SUBSCRIPTION_PRICE))]
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


# Успешная оплата
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

        cursor.execute(
            "INSERT INTO payments (user_id, amount, currency, days, status, created, paid_at) VALUES (?, ?, ?, ?, ?, ?, ?)",
            (user_id_db, str(stars_amount), "XTR", days, "paid", now_str, now_str),
        )

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
            f"🎉 **Спасибо за покупку!**\n\n"
            f"Ваша подписка PRO успешно активирована на 30 дней.\n\n"
            f"• Списано звёзд: {stars_amount} ⭐\n"
            f"• Подписка активна до: `{new_sub_until[:10]}`\n\n"
            f"Обновите страницу сайта — PRO-статус уже активен! 🚀\n"
            f"👉 {SITE_URL}"
        )
    except Exception as e:
        logger.error(f"Ошибка при сохранении платежа: {e}")
        await message.answer(f"⚠️ Оплата прошла, но произошла ошибка при обновлении базы: {e}")


# /change_password — шаг 1
@dp.message(Command("change_password"))
async def cmd_change_password(message: Message, state: FSMContext):
    data = await state.get_data()
    user_id_db = data.get("user_id_db")

    if not user_id_db:
        await message.answer("⚠️ Сначала войдите в аккаунт: /login")
        return

    await message.answer("🔑 Введите ваш **текущий пароль**:")
    await state.set_state(ChangePasswordStates.waiting_for_old_password)


@dp.message(ChangePasswordStates.waiting_for_old_password)
async def process_old_password(message: Message, state: FSMContext):
    try:
        await message.delete()
    except Exception:
        pass

    data = await state.get_data()
    user_id_db = data.get("user_id_db")

    try:
        conn = sqlite3.connect(DB_PATH)
        cursor = conn.cursor()
        cursor.execute("SELECT password_hash FROM users WHERE id=?", (user_id_db,))
        row = cursor.fetchone()
        conn.close()

        if not row:
            await message.answer("❌ Пользователь не найден.")
            await state.set_state(None)
            return

        password_hash = row[0]
        old_password = message.text

        if not (check_password_hash(password_hash, old_password) or old_password == password_hash):
            await message.answer("❌ Неверный текущий пароль. Попробуйте снова: /change\\_password")
            await state.set_state(None)
            return

        await state.update_data(old_password_verified=True)
        await message.answer("✅ Пароль подтверждён.\n\n🔒 Введите **новый пароль** (минимум 6 символов):")
        await state.set_state(ChangePasswordStates.waiting_for_new_password)

    except Exception as e:
        logger.error(f"Ошибка смены пароля (шаг 1): {e}")
        await message.answer(f"⚠️ Ошибка: {e}")
        await state.set_state(None)


@dp.message(ChangePasswordStates.waiting_for_new_password)
async def process_new_password(message: Message, state: FSMContext):
    try:
        await message.delete()
    except Exception:
        pass

    new_password = message.text

    if len(new_password) < 6:
        await message.answer("❌ Пароль слишком короткий. Минимум 6 символов. Попробуйте ещё раз:")
        return

    await state.update_data(new_password=new_password)
    await message.answer("🔒 Повторите **новый пароль** для подтверждения:")
    await state.set_state(ChangePasswordStates.waiting_for_confirm_password)


@dp.message(ChangePasswordStates.waiting_for_confirm_password)
async def process_confirm_password(message: Message, state: FSMContext):
    try:
        await message.delete()
    except Exception:
        pass

    data = await state.get_data()
    new_password = data.get("new_password")
    user_id_db = data.get("user_id_db")
    confirm = message.text

    if new_password != confirm:
        await message.answer("❌ Пароли не совпадают. Начните заново: /change\\_password")
        await state.set_state(None)
        return

    try:
        new_hash = generate_password_hash(new_password)
        conn = sqlite3.connect(DB_PATH)
        cursor = conn.cursor()
        cursor.execute("UPDATE users SET password_hash=? WHERE id=?", (new_hash, user_id_db))
        conn.commit()
        conn.close()

        await message.answer(
            "✅ **Пароль успешно изменён!**\n\n"
            "Используйте новый пароль при следующем входе на сайте и в боте."
        )
        await state.set_state(None)

    except Exception as e:
        logger.error(f"Ошибка смены пароля (шаг 3): {e}")
        await message.answer(f"⚠️ Ошибка при обновлении пароля: {e}")
        await state.set_state(None)


# /stop_subscription
@dp.message(Command("stop_subscription"))
async def cmd_stop_subscription(message: Message, state: FSMContext):
    data = await state.get_data()
    user_id_db = data.get("user_id_db")

    if not user_id_db:
        await message.answer("⚠️ Сначала войдите в аккаунт: /login")
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
            "🛑 **Подписка отменена.**\n\n"
            "Доступ к платным материалам на сайте приостановлен.\n"
            "⚠️ Согласно правилам сервиса, уплаченные средства возврату не подлежат."
        )
    except Exception as e:
        logger.error(f"Ошибка /stop_subscription: {e}")
        await message.answer(f"⚠️ Ошибка при обновлении базы данных: {e}")


async def main():
    if os.path.exists(DB_PATH):
        logger.info(f"База данных найдена: {DB_PATH}")
    else:
        logger.error(f"База данных НЕ НАЙДЕНА: {DB_PATH}")

    logger.info("Бот запущен...")
    await dp.start_polling(bot)


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except (KeyboardInterrupt, SystemExit):
        logger.info("Бот остановлен.")
