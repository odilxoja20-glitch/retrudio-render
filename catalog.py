"""Каталог проектов из папки Schemes/.

Для проекта N лежат файлы:
  N.png   — схема
  N.json  — метаданные (name, description, free, code, ...)
  N.txt   — описание (имя файла указано в json -> description)
  N.ino   — код (имя файла указано в json -> code)
"""
import json
import logging
from datetime import datetime
from config import SCHEMES_DIR

CATEGORIES = ["robotics", "smart_home", "sensors", "motors", "wireless", "iot",
              "automation", "displays", "leds", "measuring", "education", "advanced"]
DIFFICULTIES = ["easy", "medium", "hard"]

_cache = {"sig": None, "items": {}}

def _read(name):
    p = SCHEMES_DIR / name
    if p.is_file() and p.resolve().parent == SCHEMES_DIR.resolve():
        return p.read_text(encoding="utf-8")
    return ""

def loc(value, lang):
    """Строка или словарь {ru, uz, en} -> строка."""
    if isinstance(value, dict):
        return value.get(lang) or value.get("ru") or value.get("en") or next(iter(value.values()), "")
    return value or ""

def _load_item(path):
    try:
        meta = json.loads(path.read_text(encoding="utf-8"))
        pid = int(path.stem)
    except (ValueError, json.JSONDecodeError) as e:
        logging.getLogger(__name__).warning("Schemes/%s пропущен: %s", path.name, e)   # не молчим о битом JSON
        return None
    png = SCHEMES_DIR / f"{pid}.png"
    desc = meta.get("description", "")
    if isinstance(desc, dict):
        desc_texts = {k: _read(v) for k, v in desc.items()}
    else:
        desc_texts = _read(desc) if desc else ""
    published = meta.get("published")
    if not published:
        published = datetime.fromtimestamp(path.stat().st_mtime).strftime("%Y-%m-%d")
    return {
        "id": pid,
        "name": meta.get("name", f"#{pid}"),
        "description": desc_texts,
        "free": bool(meta.get("free", True)),
        "code": _read(meta["code"]) if meta.get("code") else "",
        "code_file": meta.get("code", ""),
        "board": meta.get("board", ""),
        "category": meta.get("category", ""),
        "difficulty": meta.get("difficulty", ""),
        "components": meta.get("components", []),
        "wiring": meta.get("wiring", []),          # [{"component":..,"pin":..,"board_pin":..}]
        "warnings": meta.get("warnings", []),      # строки или {ru,uz,en}
        "steps": meta.get("steps", []),
        "author": meta.get("author", ""),
        "license": meta.get("license", ""),
        "version": meta.get("version", ""),
        "changelog": meta.get("changelog", []),    # [{"version","date","note"}]
        "dependencies": meta.get("dependencies", []),
        "lang": meta.get("lang", ""),
        "verified": bool(meta.get("verified", False)),
        "published": published,
        "has_image": png.is_file(),
    }

def all_items():
    files = sorted(SCHEMES_DIR.glob("*.json"))
    sig = tuple((f.name, f.stat().st_mtime_ns) for f in files) + tuple(
        (f.name, f.stat().st_mtime_ns) for f in SCHEMES_DIR.glob("*.txt")) + tuple(
        (f.name, f.stat().st_mtime_ns) for f in SCHEMES_DIR.glob("*.ino"))
    if sig != _cache["sig"]:
        items = {}
        for f in files:
            it = _load_item(f)
            if it:
                items[it["id"]] = it
        _cache.update(sig=sig, items=items)
    return _cache["items"]

def get(pid):
    return all_items().get(pid)

def search(lang, q="", board="", category="", difficulty="", access="",
           component="", sort="new", need_code=False):
    res = []
    q = q.strip().lower()
    component = component.strip().lower()
    for it in all_items().values():
        if need_code and not it["code"]:
            continue
        if board and it["board"] != board: continue
        if category and it["category"] != category: continue
        if difficulty and it["difficulty"] != difficulty: continue
        if access == "free" and not it["free"]: continue
        if access == "paid" and it["free"]: continue
        if component and not any(component in str(c).lower() for c in it["components"]): continue
        if q:
            d = it["description"]
            d = " ".join(d.values()) if isinstance(d, dict) else d
            n = it["name"]
            n = " ".join(n.values()) if isinstance(n, dict) else n
            hay = " ".join([n, d, " ".join(map(str, it["components"])), it["code_file"]]).lower()
            if q not in hay: continue
        res.append(it)
    if sort == "name":
        res.sort(key=lambda i: loc(i["name"], lang).lower())
    else:
        res.sort(key=lambda i: (i["published"], i["id"]), reverse=True)
    return res

def boards():
    return sorted({i["board"] for i in all_items().values() if i["board"]})

def similar(item, limit=3):
    out = [i for i in all_items().values() if i["id"] != item["id"]
           and (i["category"] == item["category"] or i["board"] == item["board"])]
    return sorted(out, key=lambda i: i["published"], reverse=True)[:limit]
