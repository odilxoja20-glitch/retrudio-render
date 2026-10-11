"""Генерирует упрощённые PNG-иллюстрации для демо-проектов в Schemes/.
Замените их своими схемами (например, из Fritzing) — имена файлов 1.png, 2.png, 3.png."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

OUT = Path(__file__).resolve().parent.parent / "Schemes"
FONT = ImageFont.load_default(size=18)

def box(d, xy, label, color="#2f7dff"):
    d.rounded_rectangle(xy, 12, fill="#0e1524", outline=color, width=3)
    d.text(((xy[0]+xy[2])//2, xy[1]+14), label, fill="#e6ecf8", font=FONT, anchor="mt")

def wire(d, pts, color="#3ee0ff", label=None):
    d.line(pts, fill=color, width=4, joint="curve")
    if label:
        x, y = pts[len(pts)//2]
        d.text((x, y-22), label, fill="#8b97b0", font=FONT, anchor="mm")

def make(name, parts, wires):
    img = Image.new("RGB", (800, 500), "#0a1220")
    d = ImageDraw.Draw(img)
    for w in wires: wire(d, *w)
    for xy, label, color in parts: box(d, xy, label, color)
    img.save(OUT / name)

make("1.png",
     [((40, 120, 280, 400), "Arduino UNO", "#2f7dff"),
      ((440, 120, 560, 180), "R 220", "#8b5cf6"), ((640, 120, 760, 180), "LED", "#8b5cf6")],
     [([(280, 150), (440, 150)], "#3ee0ff", "D13"), ([(560, 150), (640, 150)], "#3ee0ff", None),
      ([(700, 180), (700, 440), (160, 440), (160, 400)], "#9aa4b8", "GND")])
make("2.png",
     [((40, 120, 280, 400), "Arduino UNO", "#2f7dff"), ((520, 120, 760, 400), "HC-SR04", "#8b5cf6")],
     [([(280, 160), (520, 160)], "#ef4444", "5V → VCC"), ([(280, 220), (520, 220)], "#3ee0ff", "D9 → TRIG"),
      ([(280, 280), (520, 280)], "#34d399", "D10 → ECHO"), ([(280, 340), (520, 340)], "#9aa4b8", "GND → GND")])
make("3.png",
     [((40, 120, 280, 400), "Arduino UNO", "#2f7dff"), ((520, 160, 760, 360), "Servo SG90", "#8b5cf6")],
     [([(280, 200), (520, 200)], "#ef4444", "5V → VCC"), ([(280, 260), (520, 260)], "#f59e0b", "D9 → SIG"),
      ([(280, 320), (520, 320)], "#9aa4b8", "GND → GND")])
print("PNG созданы в", OUT)
