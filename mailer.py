"""Отправка кода подтверждения через сервис sc5mails (GET-запрос по URL-шаблону из .env)."""
import logging
import urllib.error
import urllib.request
from urllib.parse import quote

import config

log = logging.getLogger(__name__)


def build_url(to, code, name="-", ip="-", device="-"):
    """Каждое значение кодируется как отдельный сегмент пути (@ -> %40, / -> %2F, кириллица и пробелы безопасны).
    Пустых сегментов не бывает: вместо пустого значения подставляется «-»."""
    q = lambda v: quote(str(v).strip() or "-", safe="")
    return config.SC5MAILS_URL_TEMPLATE.format(
        base=config.SC5MAILS_BASE_URL, company_name=q(config.SC5MAILS_COMPANY), code=q(code), to_email=q(to),
        subject=q(config.SC5MAILS_SUBJECT), user_name=q(name), user_ip=q(ip), device=q(device[:60]),
        support_link=q(config.SC5MAILS_SUPPORT_LINK), expire_time=q(f"{config.CODE_TTL_MIN} min"))


def send_code(to, code, name="-", ip="-", device="-"):
    """True — сервис ответил 2xx. Сам код и URL в лог не пишутся."""
    try:
        req = urllib.request.Request(build_url(to, code, name, ip, device), headers={"User-Agent": f"{config.PROJECT_NAME}/1.0"})
        with urllib.request.urlopen(req, timeout=config.SC5MAILS_TIMEOUT) as r:
            return 200 <= r.status < 300
    except urllib.error.HTTPError as e:
        log.error("sc5mails: HTTP %s", e.code)
    except Exception as e:
        log.error("sc5mails: %s", type(e).__name__)
    return False
