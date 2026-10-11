import subprocess
import sys
import threading

def stream_logs(process, prefix):
    """Shtyvaet logi iz protsessa i vyvodit ich s krasivym prefiksom."""
    for line in iter(process.stdout.readline, ''):
        if line:
            print(f"[{prefix}] {line.strip()}")

def main():
    print("🚀 Zapusk Retrudio Ecosystem (Flask + Telegram Bot)...")

    # Zapuskaem app.py (Flask)
    app_process = subprocess.Popen(
        [sys.executable, "app.py"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1
    )

    # Zapuskaem bot.py (Aiogram)
    bot_process = subprocess.Popen(
        [sys.executable, "bot.py"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1
    )

    # Zapuskaem potoki dlya chteniya logov v realnom vremeni
    threading.Thread(target=stream_logs, args=(app_process, "FLASK"), daemon=True).start()
    threading.Thread(target=stream_logs, args=(bot_process, "BOT"), daemon=True).start()

    try:
        # Derjim glavnyy protsess aktivnym
        app_process.wait()
        bot_process.wait()
    except KeyboardInterrupt:
        print("\n🛑 Ostanovka vseh servisev...")
        app_process.terminate()
        bot_process.terminate()
        print("✅ Vse protsessy uspeshno ostanovleny.")

if __name__ == "__main__":
    main()