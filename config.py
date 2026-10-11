import os
from pathlib import Path
from dotenv import load_dotenv

BASE_DIR = Path(__file__).resolve().parent
load_dotenv(BASE_DIR / ".env")

PROJECT_NAME = os.getenv("PROJECT_NAME", "Retrudio")   # единая точка смены названия
SECRET_KEY = os.getenv("SECRET_KEY", "dev-only-change-me")
APP_ENV = os.getenv("APP_ENV", "development")
SCHEMES_DIR = BASE_DIR / "Schemes"
DB_PATH = Path(os.getenv("DB_PATH", BASE_DIR / "data" / "app.db"))
PER_PAGE = 12
DONATE_URL = os.getenv("DONATE_URL", "")
CHECKOUT_URL = os.getenv("CHECKOUT_URL", "")
PAYMENT_WEBHOOK_SECRET = os.getenv("PAYMENT_WEBHOOK_SECRET", "")
CONTACT_EMAIL = os.getenv("CONTACT_EMAIL", "")
LANGS = ("ru", "uz", "en")

# --- FreeKassa (значения берутся ТОЛЬКО из .env, в код секреты не вписывать) ---
SITE_URL = os.getenv("SITE_URL", "").rstrip("/")
FREEKASSA_SHOP_ID = os.getenv("FREEKASSA_SHOP_ID", "")
FREEKASSA_SECRET1 = os.getenv("FREEKASSA_SECRET1", "")   # для подписи платёжной формы
FREEKASSA_SECRET2 = os.getenv("FREEKASSA_SECRET2", "")   # для проверки оповещений
FREEKASSA_PAY_URL = os.getenv("FREEKASSA_PAY_URL", "https://pay.fk.money/")   # по документации FreeKassa
# IP серверов FreeKassa из их документации. Чтобы отключить проверку (только для тестов): FREEKASSA_ALLOWED_IPS=none
_DEFAULT_IPS = "168.119.157.136,168.119.60.227,178.154.197.79,51.250.54.238"
_ips = os.getenv("FREEKASSA_ALLOWED_IPS", "").strip() or _DEFAULT_IPS
FREEKASSA_ALLOWED_IPS = [] if _ips.lower() == "none" else [i.strip() for i in _ips.split(",") if i.strip()]
TRUST_PROXY = os.getenv("TRUST_PROXY", "0") == "1"   # 1 = брать реальный IP из X-Real-IP (за nginx)
FREEKASSA_CURRENCIES = ("TelegramStars", "USD", "EUR", "UAH", "KZT")   # список из документации FreeKassa
SUBSCRIPTION_PRICE = os.getenv("SUBSCRIPTION_PRICE", "")        # например 5.00
SUBSCRIPTION_CURRENCY = os.getenv("SUBSCRIPTION_CURRENCY", "RUB")
SUBSCRIPTION_DAYS = int(os.getenv("SUBSCRIPTION_DAYS", "30"))
FREEKASSA_READY = bool(FREEKASSA_SHOP_ID and FREEKASSA_SECRET1 and FREEKASSA_SECRET2 and SUBSCRIPTION_PRICE
                       and SUBSCRIPTION_CURRENCY in FREEKASSA_CURRENCIES)

# --- sc5mails: письма с кодом подтверждения при регистрации ---
SC5MAILS_BASE_URL = os.getenv("SC5MAILS_BASE_URL", "https://sc5mails.onrender.com").rstrip("/")
SC5MAILS_COMPANY = os.getenv("SC5MAILS_COMPANY", "") or PROJECT_NAME     # company_name в сервисе; по умолчанию название проекта
SC5MAILS_SERVICE = os.getenv("SC5MAILS_SERVICE", PROJECT_NAME)           # «имя на сервисе»
SC5MAILS_SUBJECT = os.getenv("SC5MAILS_SUBJECT", f"{PROJECT_NAME}: код подтверждения")
# Шаблон из документации сервиса (Swagger). Значения подставляются и кодируются автоматически
SC5MAILS_URL_TEMPLATE = os.getenv("SC5MAILS_URL_TEMPLATE",
    "{base}/api/{company_name}/{code}/{to_email}/{subject}/{user_name}/{user_ip}/{device}/{support_link}/{expire_time}/done")
SC5MAILS_SUPPORT_LINK = os.getenv("SC5MAILS_SUPPORT_LINK", "") or SITE_URL or CONTACT_EMAIL or "-"
SC5MAILS_TIMEOUT = int(os.getenv("SC5MAILS_TIMEOUT", "40"))              # бесплатный Render просыпается до ~50 с
SC5MAILS_READY = bool(SC5MAILS_COMPANY)
CODE_TTL_MIN = 10
CODE_MAX_ATTEMPTS = 5
CODE_RESEND_SEC = 60
