import sqlite3
from datetime import datetime, timezone
from flask import g
from config import DB_PATH

SCHEMA = """
CREATE TABLE IF NOT EXISTS users(
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  email TEXT UNIQUE NOT NULL,
  name TEXT NOT NULL,
  password_hash TEXT NOT NULL,
  subscription_until TEXT,
  username TEXT UNIQUE,
  is_admin INTEGER NOT NULL DEFAULT 0,
  created TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE IF NOT EXISTS favorites(
  user_id INTEGER NOT NULL, project_id INTEGER NOT NULL,
  PRIMARY KEY(user_id, project_id)
);
CREATE TABLE IF NOT EXISTS comments(
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  project_id INTEGER NOT NULL, user_id INTEGER NOT NULL,
  body TEXT NOT NULL, rating INTEGER,
  created TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE IF NOT EXISTS designs(
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  user_id INTEGER NOT NULL, title TEXT NOT NULL,
  data TEXT NOT NULL, public INTEGER NOT NULL DEFAULT 0,
  created TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE IF NOT EXISTS pending_registrations(
  email TEXT PRIMARY KEY, name TEXT NOT NULL, password_hash TEXT NOT NULL,
  code_hash TEXT NOT NULL, expires TEXT NOT NULL,
  attempts INTEGER NOT NULL DEFAULT 0, last_sent TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS payments(
  id INTEGER PRIMARY KEY AUTOINCREMENT,          -- это и есть MERCHANT_ORDER_ID
  user_id INTEGER NOT NULL,
  amount TEXT NOT NULL, currency TEXT NOT NULL, days INTEGER NOT NULL,
  status TEXT NOT NULL DEFAULT 'pending',        -- pending | paid
  fk_intid TEXT,
  created TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
  paid_at TEXT
);
"""

def get_db():
    if "db" not in g:
        g.db = sqlite3.connect(DB_PATH)
        g.db.row_factory = sqlite3.Row
    return g.db

def close_db(_=None):
    db = g.pop("db", None)
    if db:
        db.close()

def init_db():
    DB_PATH.parent.mkdir(exist_ok=True)
    con = sqlite3.connect(DB_PATH)
    con.executescript(SCHEMA)
    cols = {r[1] for r in con.execute("PRAGMA table_info(users)")}      # миграция старых баз
    if "username" not in cols:
        con.execute("ALTER TABLE users ADD COLUMN username TEXT")
        con.execute("CREATE UNIQUE INDEX IF NOT EXISTS idx_users_username ON users(username)")
    if "is_admin" not in cols:
        con.execute("ALTER TABLE users ADD COLUMN is_admin INTEGER NOT NULL DEFAULT 0")
    con.commit()
    con.close()


def utcnow():
    return datetime.now(timezone.utc).replace(tzinfo=None)
