"""Полная проверка: python tests/test_app.py  (использует временную БД и тестовые ключи)"""
import hashlib, json, os, re, sys, tempfile
from datetime import datetime, timezone
from urllib.parse import parse_qs, urlparse

tmp = tempfile.mkdtemp()
os.environ.update(DB_PATH=os.path.join(tmp, "t.db"), FREEKASSA_SHOP_ID="12345", FREEKASSA_SECRET1="s1-test",
                  FREEKASSA_SECRET2="s2-test", SUBSCRIPTION_PRICE="5.00", SUBSCRIPTION_CURRENCY="RUB",
                  SUBSCRIPTION_DAYS="30", SECRET_KEY="x" * 32)
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import app as A
import config

fails = []
def ok(label, cond):
    print("OK  " if cond else "FAIL", label)
    if not cond: fails.append(label)
md5 = lambda s: hashlib.md5(s.encode()).hexdigest()
csrf = lambda h: re.search(r'name="csrf" value="([^"]+)"', h).group(1)
c = A.app.test_client()

# --- страницы и каталог
for u in ["/", "/projects", "/library", "/builder", "/community", "/pricing", "/donate", "/login", "/register",
          "/project/1", "/project/2", "/project/3", "/scheme-image/1", "/payment/success", "/payment/fail"]:
    ok(f"GET {u}", c.get(u).status_code == 200)
ok("404 для несуществующего проекта", c.get("/project/999").status_code == 404)
ok("фильтр free", b"/project/3" not in c.get("/projects?access=free").data)
ok("фильтр paid", b"/project/3" in c.get("/projects?access=paid").data and b"/project/1" not in c.get("/projects?access=paid").data)
ok("поиск по компоненту", b"/project/2" in c.get("/projects?component=hc-sr04").data and b"/project/1" not in c.get("/projects?component=hc-sr04").data)
ok("фильтр категории", b"/project/1" in c.get("/projects?category=leds").data and b"/project/2" not in c.get("/projects?category=leds").data)
ok("языки", "Bosh sahifa".encode() in c.get("/?lang=uz").data and b"Home" in c.get("/?lang=en").data)
ok("платный проект закрыт (страница и скачивание)", "доступен по подписке".encode() in c.get("/project/3?lang=ru").data and c.get("/project/3/download").status_code == 403)
ok("код платного проекта не виден в HTML", b"servo.attach" not in c.get("/project/3").data)
ok("скачивание .ino", b"LED_PIN" in c.get("/project/1/download").data and "attachment" in c.get("/project/1/download").headers["Content-Disposition"])
ok("секреты не попадают в страницы", all(x not in c.get("/pricing").data for x in (b"s1-test", b"s2-test")))

# --- безопасность
ok("POST без CSRF -> 400", c.post("/login", data={"email": "a@b.co", "password": "x"}).status_code == 400)
tok = csrf(c.get("/register").data.decode())
ok("регистрация (слабый пароль отклонён)", c.post("/register", data={"csrf": tok, "name": "T", "email": "p@example.com", "password": "short"}).status_code == 200)
# --- регистрация с кодом из письма (sc5mails подменён заглушкой)
import mailer
sent = []
config.SC5MAILS_READY = True
mailer.send_code = lambda to, code, *a: (sent.append((to, code)) or True)
reg = {"csrf": tok, "name": "T", "email": "p@example.com", "password": "password123"}
r = c.post("/register", data=reg)
ok("регистрация -> страница ввода кода", r.status_code == 302 and r.headers["Location"].endswith("/verify") and len(sent) == 1)
import sqlite3 as _sq
_q = lambda sql: (lambda k: (k.execute(sql).fetchone(), k.close())[0])(_sq.connect(config.DB_PATH))
ok("пользователь НЕ создан до подтверждения", _q("SELECT 1 FROM users WHERE email='p@example.com'") is None and _q("SELECT 1 FROM pending_registrations WHERE email='p@example.com'") is not None)
ok("страница /verify", b'name="code"' in c.get("/verify").data)
ok("повтор регистрации сразу — кулдаун", "Подождите минуту".encode() in c.post("/register", data=reg).data and len(sent) == 1)
tv = csrf(c.get("/verify").data.decode())
r = c.post("/verify", data={"csrf": tv, "code": "000000" if sent[0][1] != "000000" else "111111"})
ok("неверный код отклонён", r.status_code == 200 and "Неверный код".encode() in r.data)
ok("всё ещё не залогинен", c.get("/favorites").status_code == 302)
ok("resend в кулдауне не шлёт", c.post("/verify/resend", data={"csrf": tv}).status_code == 302 and len(sent) == 1)
A.get_db  # (кулдаун сбрасываем напрямую в БД)
import sqlite3
con = sqlite3.connect(config.DB_PATH); con.execute("UPDATE pending_registrations SET last_sent='2000-01-01T00:00:00'"); con.commit(); con.close()
c.post("/verify/resend", data={"csrf": tv})
ok("resend отправил новый код", len(sent) == 2)
ok("старый код больше не подходит", sent[0][1] == sent[1][1] or "Неверный код".encode() in c.post("/verify", data={"csrf": tv, "code": sent[0][1]}).data)
r = c.post("/verify", data={"csrf": tv, "code": sent[1][1][:3] + " " + sent[1][1][3:]})
ok("верный код -> аккаунт создан и вход выполнен", r.status_code == 302 and c.get("/favorites").status_code == 200)
_c3 = A.app.test_client()
ok("повторная регистрация той же почты отклонена", "уже зарегистрирована".encode() in _c3.post("/register", data={**reg, "csrf": csrf(_c3.get("/register").data.decode())}).data and _q("SELECT 1 FROM pending_registrations WHERE email='p@example.com'") is None)
# 5 неверных попыток сбрасывают регистрацию
cb = A.app.test_client(); tb = csrf(cb.get("/register").data.decode())
cb.post("/register", data={"csrf": tb, "name": "B", "email": "b@example.com", "password": "password123"})
tb = csrf(cb.get("/verify").data.decode()); good = sent[-1][1]; bad = "000000" if good != "000000" else "111111"
for _ in range(5): cb.post("/verify", data={"csrf": tb, "code": bad})
r = cb.post("/verify", data={"csrf": tb, "code": good})
ok("после 5 неверных попыток верный код уже не принимается", r.status_code == 302 and r.headers["Location"].endswith("/register"))
# сбой сервиса писем
mailer.send_code = lambda to, code, *a: False
cf = A.app.test_client(); tf = csrf(cf.get("/register").data.decode())
r = cf.post("/register", data={"csrf": tf, "name": "F", "email": "f@example.com", "password": "password123"})
ok("сбой письма -> ошибка, регистрация не создаётся", r.status_code == 200 and "Не удалось отправить".encode() in r.data)
# формат URL sc5mails
import importlib; importlib.reload(mailer)
config.SC5MAILS_COMPANY = "Retrudio"; config.SC5MAILS_SUPPORT_LINK = "https://site.uz/help"
u = mailer.build_url("toirovrahmatulloh5@gmail.com", "123456", "Ali Valiev", "1.2.3.4", "Windows")
ok("URL по шаблону сервиса", u == "https://sc5mails.onrender.com/api/Retrudio/123456/toirovrahmatulloh5%40gmail.com/"
   + __import__("urllib.parse").parse.quote(config.SC5MAILS_SUBJECT, safe="") + "/Ali%20Valiev/1.2.3.4/Windows/https%3A%2F%2Fsite.uz%2Fhelp/10%20min/done")
ok("пустые значения -> «-», сегментов всегда 9 после api", mailer.build_url("a@b.co", "1", "", "", "").count("/-/") >= 2)
c2 = A.app.test_client(); t2 = csrf(c2.get("/login").data.decode())
for bad in ("//evil.com", "/\\evil.com", "https://evil.com"):
    r = c2.post("/login?next=" + bad, data={"csrf": t2, "email": "p@example.com", "password": "password123"})
    ok(f"open redirect заблокирован ({bad})", r.status_code == 302 and "evil" not in r.headers["Location"])
    t2 = csrf(c2.get("/login").data.decode()) if b'name="csrf"' in c2.get("/login").data else t2
    c2.post("/logout", data={"csrf": csrf(c2.get("/pricing").data.decode())})
    t2 = csrf(c2.get("/login").data.decode())
ok("неверный пароль -> сообщение об ошибке", "Неверная почта или пароль".encode() in c2.post("/login", data={"csrf": t2, "email": "p@example.com", "password": "wrongwrong"}, follow_redirects=True).data)

# --- избранное, комментарии, схемы
page = c.get("/project/1").data.decode(); tok = csrf(page)
ok("favorite", c.post("/project/1/favorite", data={"csrf": tok}).status_code == 302)
ok("popular на главной", b"/project/1" in c.get("/").data)
ok("comment", c.post("/project/1/comment", data={"csrf": tok, "body": "<b>Работает</b>", "rating": "5"}).status_code == 302)
p = c.get("/project/1").data.decode()
ok("комментарий экранирован (XSS)", "&lt;b&gt;" in p and "<b>Работает" not in p)
ok("оценка и подсветка кода", "★ 5.0" in p and 'class="hl"' in p)
h = {"X-CSRF-Token": tok, "Content-Type": "application/json"}
r = c.post("/api/designs", headers=h, data=json.dumps({"title": "t1", "public": True, "data": {"parts": [], "wires": []}}))
did = r.get_json()["id"]
ok("схема сохранена и в сообществе", r.status_code == 200 and b"t1" in c.get("/community").data)
ok("публичная схема доступна всем", A.app.test_client().get(f"/api/designs/{did}").status_code == 200)  # публичная
r = c.post("/api/designs", headers=h, data=json.dumps({"title": "priv", "public": False, "data": {}}))
pid = r.get_json()["id"]
ok("приватная схема скрыта от других", A.app.test_client().get(f"/api/designs/{pid}").status_code == 404)

# --- оплата FreeKassa
page = c.get("/pricing").data.decode()
ok("кнопка оплаты", "/pay/subscribe" in page)
r = c.post("/pay/subscribe", data={"csrf": csrf(page)})
loc = r.headers["Location"]; q = {k: v[0] for k, v in parse_qs(urlparse(loc).query).items()}
ok("редирект на pay.fk.money", loc.startswith("https://pay.fk.money/?"))
ok("подпись формы = md5(shop:сумма:секрет1:валюта:заказ)", q["s"] == md5(f"12345:5.00:s1-test:RUB:{q['o']}") and q["m"] == "12345" and q["oa"] == "5.00")
oid = q["o"]
def notify(client=None, headers=None, env=None, **kw):
    d = {"MERCHANT_ID": "12345", "AMOUNT": "5.00", "MERCHANT_ORDER_ID": oid, "intid": "777"}; d.update(kw)
    d.setdefault("SIGN", md5(f"{d['MERCHANT_ID']}:{d['AMOUNT']}:s2-test:{d['MERCHANT_ORDER_ID']}"))
    return (client or c).post("/payment/notify", data=d, headers=headers or {}, environ_overrides=env or {})
config.FREEKASSA_ALLOWED_IPS[:] = ["168.119.157.136", "168.119.60.227", "178.154.197.79", "51.250.54.238"]
ok("чужой IP -> 403", notify().status_code == 403)
ok("X-Real-IP игнорируется без TRUST_PROXY", notify(headers={"X-Real-IP": "168.119.157.136"}).status_code == 403)
ok("плохая подпись -> 400", notify(env={"REMOTE_ADDR": "168.119.157.136"}, SIGN="bad").status_code == 400)
ok("чужой магазин -> 400", notify(env={"REMOTE_ADDR": "168.119.157.136"}, MERCHANT_ID="999").status_code == 400)
ok("неверная сумма -> 400", notify(env={"REMOTE_ADDR": "168.119.157.136"}, AMOUNT="1.00").status_code == 400)
ok("несуществующий заказ -> 404", notify(env={"REMOTE_ADDR": "168.119.157.136"}, MERCHANT_ORDER_ID="0").status_code == 404)
ok("подписки ещё нет", c.get("/project/3/download").status_code == 403)
r = notify(env={"REMOTE_ADDR": "168.119.157.136"}); ok("валидное оповещение -> YES", r.data == b"YES")
until1 = A.get_db.__globals__["sqlite3"].connect(config.DB_PATH).execute("SELECT subscription_until FROM users").fetchone()[0]
ok("подписка выдана на 30 дней", c.get("/project/3/download").status_code == 200 and 29 <= (datetime.fromisoformat(until1) - datetime.now(timezone.utc).replace(tzinfo=None)).days <= 30)
ok("повтор -> YES", notify(env={"REMOTE_ADDR": "168.119.157.136"}).data == b"YES")
until2 = A.get_db.__globals__["sqlite3"].connect(config.DB_PATH).execute("SELECT subscription_until FROM users").fetchone()[0]
ok("повтор не продлевает подписку второй раз", until1 == until2)
config.TRUST_PROXY = True
ok("TRUST_PROXY: X-Real-IP из списка принят", notify(headers={"X-Real-IP": "51.250.54.238"}).data == b"YES")
config.TRUST_PROXY = False
ok("GET-оповещение тоже работает", c.get("/payment/notify", query_string={"MERCHANT_ID": "12345", "AMOUNT": "5.00", "MERCHANT_ORDER_ID": oid, "SIGN": md5(f"12345:5.00:s2-test:{oid}")}, environ_overrides={"REMOTE_ADDR": "178.154.197.79"}).data == b"YES")
ok("success/fail принимают POST без CSRF", c.post("/payment/success").status_code == 200 and c.post("/payment/fail").status_code == 200)
ok("success сам подписку не выдаёт (другой пользователь)", A.app.test_client().post("/payment/success").status_code == 200 and A.app.test_client().get("/project/3/download").status_code == 403)
ok("валюта UZS не поддерживается", "UZS" not in config.FREEKASSA_CURRENCIES)

print("\nПровалено:", len(fails)); sys.exit(1 if fails else 0)
