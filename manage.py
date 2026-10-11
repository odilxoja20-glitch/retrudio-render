"""Админ-команды:
  python manage.py grant-sub user@mail.com 30   — выдать подписку вручную
  python manage.py create-admin username password — создать/обновить администратора (все проекты открыты)
  python manage.py payment-urls                  — адреса для кабинета FreeKassa (из SITE_URL)
  python manage.py payment-check                 — отправить тестовое подписанное оповещение на ваш сайт
"""
import hashlib
import sqlite3
import sys
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime, timedelta

import config
from database import init_db, utcnow


def grant(email, days):
    con = sqlite3.connect(config.DB_PATH)
    row = con.execute("SELECT subscription_until FROM users WHERE email=?", (email.lower(),)).fetchone()
    if not row:
        print("Пользователь не найден"); return
    now = utcnow()
    cur = datetime.fromisoformat(row[0]) if row[0] else now
    new = max(cur, now) + timedelta(days=days)
    con.execute("UPDATE users SET subscription_until=? WHERE email=?", (new.isoformat(), email.lower()))
    con.commit(); print("Подписка до", new.date())


def create_admin(username, password):
    from werkzeug.security import generate_password_hash
    username = username.strip().lower()
    if len(password) < 8:
        print("Пароль короче 8 символов"); return
    con = sqlite3.connect(config.DB_PATH)
    h = generate_password_hash(password)
    if con.execute("SELECT 1 FROM users WHERE username=?", (username,)).fetchone():
        con.execute("UPDATE users SET password_hash=?, is_admin=1 WHERE username=?", (h, username))
    else:
        con.execute("INSERT INTO users(email,name,password_hash,username,is_admin) VALUES(?,?,?,?,1)",
                    (f"{username}@admin.local", username, h, username))
    con.commit(); print("Администратор готов:", username)


def payment_urls():
    if not config.SITE_URL:
        print("Укажите SITE_URL в .env, например SITE_URL=https://ваш-домен.uz\n"
              "(для теста без домена: запустите ngrok и впишите его https-адрес)")
        return None
    if not config.SITE_URL.startswith("https://"):
        print("⚠ SITE_URL без https — платёжная система может отказаться отправлять оповещения.\n")
    base = config.SITE_URL
    print("Вставьте в кабинет FreeKassa:\n")
    print(f"  URL оповещения         : {base}/payment/notify    метод: POST")
    print(f"  URL успешной оплаты    : {base}/payment/success   метод: GET")
    print(f"  URL возврата при неудаче: {base}/payment/fail      метод: GET")
    return base


def payment_check():
    base = payment_urls()
    if not base:
        return
    if config.SUBSCRIPTION_CURRENCY not in config.FREEKASSA_CURRENCIES:
        print(f"\n❌ Валюта {config.SUBSCRIPTION_CURRENCY} не поддерживается FreeKassa. Доступны: {', '.join(config.FREEKASSA_CURRENCIES)}"); return
    if not config.FREEKASSA_READY:
        print("\nЗаполните FREEKASSA_SHOP_ID, FREEKASSA_SECRET1, FREEKASSA_SECRET2 и SUBSCRIPTION_PRICE в .env"); return
    order, amount = "0", config.SUBSCRIPTION_PRICE        # заказа 0 не существует — деньги/подписки не затрагиваются
    sign = hashlib.md5(f"{config.FREEKASSA_SHOP_ID}:{amount}:{config.FREEKASSA_SECRET2}:{order}".encode()).hexdigest()
    data = urllib.parse.urlencode({"MERCHANT_ID": config.FREEKASSA_SHOP_ID, "AMOUNT": amount,
                                   "MERCHANT_ORDER_ID": order, "SIGN": sign, "intid": "test"}).encode()
    print("\nОтправляю тестовое оповещение…")
    try:
        r = urllib.request.urlopen(urllib.request.Request(base + "/payment/notify", data=data), timeout=15)
        code, body = r.status, r.read().decode()
    except urllib.error.HTTPError as e:
        code, body = e.code, e.read().decode()
    except Exception as e:
        print("Сайт недоступен:", e); return
    hints = {
        (404, "order not found"): "✅ Подпись и ID магазина приняты (заказ 0 — тестовый, поэтому «не найден»). Всё настроено верно.",
        (400, "wrong sign"): "❌ Неверная подпись: проверьте FREEKASSA_SECRET2 и FREEKASSA_SHOP_ID на сервере.",
        (403, "hacking attempt"): "⚠ Сервер отклонил ваш IP из-за FREEKASSA_ALLOWED_IPS — для проверки с вашего компьютера временно поставьте FREEKASSA_ALLOWED_IPS=none, перезапустите сайт и повторите (потом верните).",
        (503, "not configured"): "❌ На сервере не заполнены переменные FREEKASSA_* в .env.",
    }
    print(f"Ответ: {code} {body!r}\n" + hints.get((code, body), "Неожиданный ответ — смотрите лог сервера."))


if __name__ == "__main__":
    init_db()
    cmd = sys.argv[1] if len(sys.argv) > 1 else ""
    if cmd == "grant-sub" and len(sys.argv) == 4:
        grant(sys.argv[2], int(sys.argv[3]))
    elif cmd == "create-admin" and len(sys.argv) == 4:
        create_admin(sys.argv[2], sys.argv[3])
    elif cmd == "payment-urls":
        payment_urls()
    elif cmd == "payment-check":
        payment_check()
    else:
        print(__doc__)
