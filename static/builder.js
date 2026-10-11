(() => {
  const B = window.BUILDER;
  const NS = "http://www.w3.org/2000/svg";
  const svg = document.getElementById("canvas");
  const ST = 18, HEAD = 26;

  // l = контакты слева, r = справа
  const DEFS = {
    uno:   {label: "Arduino UNO", w: 150, l: ["A0","A1","A2","A3","A4","A5","5V","3V3","GND","VIN"], r: ["D0","D1","D2","D3","D4","D5","D6","D7","D8","D9","D10","D11","D12","D13"]},
    nano:  {label: "Arduino Nano", w: 130, l: ["A0","A1","A2","A3","A4","A5","5V","3V3","GND","VIN"], r: ["D2","D3","D4","D5","D6","D7","D8","D9","D10","D11","D12","D13"]},
    esp32: {label: "ESP32", w: 140, l: ["3V3","GND","VIN","EN"], r: ["GPIO2","GPIO4","GPIO5","GPIO12","GPIO13","GPIO14","GPIO15","GPIO16","GPIO17","GPIO18","GPIO19","GPIO21 SDA","GPIO22 SCL","GPIO23","GPIO25","GPIO26","GPIO27","GPIO32","GPIO33","GPIO34"]},
    breadboard: {label: "Breadboard (упрощённо)", w: 140, l: ["+","−","a1","a2","a3","a4","a5","a6","a7","a8"], r: ["+","−","b1","b2","b3","b4","b5","b6","b7","b8"]},
    led:   {label: "LED", w: 90, l: ["A (+)"], r: ["K (−)"]},
    res:   {label: "Резистор", w: 90, l: ["1"], r: ["2"]},
    btn:   {label: "Кнопка", w: 90, l: ["1"], r: ["2"]},
    pot:   {label: "Потенциометр", w: 120, l: ["VCC","GND"], r: ["OUT"]},
    hcsr04:{label: "HC-SR04", w: 110, l: ["VCC","GND"], r: ["TRIG","ECHO"]},
    sg90:  {label: "Серво SG90", w: 110, l: ["VCC","GND"], r: ["SIG"]},
    hc05:  {label: "HC-05", w: 110, l: ["VCC","GND"], r: ["TX","RX"]},
    l298n: {label: "L298N", w: 130, l: ["12V","GND","5V","ENA","IN1","IN2","IN3","IN4","ENB"], r: ["OUT1","OUT2","OUT3","OUT4"]},
    dcm:   {label: "DC-мотор", w: 100, l: ["M+"], r: ["M−"]},
    temp:  {label: "Датчик температуры", w: 130, l: ["VCC","GND"], r: ["OUT"]},
    lcd:   {label: "LCD (I2C)", w: 110, l: ["VCC","GND"], r: ["SDA","SCL"]},
    buzzer:{label: "Пьезозуммер", w: 110, l: ["+"], r: ["−"]},
  };

  let state = {parts: [], wires: [], view: {x: 20, y: 20, k: 1}};
  let selected = null, pending = null, uid = 1, currentId = B.designId, mine = true;
  const $ = (id) => document.getElementById(id);

  const height = (d) => HEAD + Math.max(d.l.length, d.r.length) * ST + 10;
  const pinPos = (part, side, i) => {
    const d = DEFS[part.type];
    return {x: part.x + (side === "r" ? d.w : 0), y: part.y + HEAD + i * ST + ST / 2};
  };
  const findPin = (ref) => {
    const part = state.parts.find(p => p.id === ref.part); if (!part) return null;
    const d = DEFS[part.type];
    let i = d.l.indexOf(ref.pin);  if (i >= 0 && ref.side === "l") return pinPos(part, "l", i);
    i = d.r.indexOf(ref.pin);      if (i >= 0) return pinPos(part, "r", i);
    return null;
  };
  const el = (name, attrs = {}, parent) => {
    const e = document.createElementNS(NS, name);
    for (const k in attrs) e.setAttribute(k, attrs[k]);
    if (parent) parent.appendChild(e); return e;
  };

  function render() {
    svg.innerHTML = "";
    const g = el("g", {transform: `translate(${state.view.x} ${state.view.y}) scale(${state.view.k})`}, svg);
    for (const p of state.parts) {
      const d = DEFS[p.type], h = height(d);
      const grp = el("g", {}, g);
      el("rect", {x: p.x, y: p.y, width: d.w, height: h, rx: 8, fill: "#0e1524",
        stroke: selected?.kind === "part" && selected.id === p.id ? "#3ee0ff" : "#2f7dff", "stroke-width": 1.5, style: "cursor:move"}, grp)
        .addEventListener("pointerdown", (e) => startDrag(e, p));
      const t = el("text", {x: p.x + d.w / 2, y: p.y + 17, "text-anchor": "middle", fill: "#e6ecf8", "font-size": 12, "pointer-events": "none"}, grp);
      t.textContent = d.label;
      ["l", "r"].forEach(side => d[side].forEach((name, i) => {
        const pos = pinPos(p, side, i);
        const isP = pending && pending.part === p.id && pending.pin === name && pending.side === side;
        const c = el("circle", {cx: pos.x, cy: pos.y, r: 5, fill: isP ? "#3ee0ff" : "#142033", stroke: "#3ee0ff", style: "cursor:crosshair"}, grp);
        c.addEventListener("pointerdown", (e) => { e.stopPropagation(); clickPin({part: p.id, pin: name, side}); });
        const lab = el("text", {x: pos.x + (side === "l" ? 10 : -10), y: pos.y + 4, "text-anchor": side === "l" ? "start" : "end",
          fill: "#8b97b0", "font-size": 10, "pointer-events": "none"}, grp);
        lab.textContent = name;
      }));
    }
    state.wires.forEach((w, idx) => {
      const a = findPin(w.from), b = findPin(w.to); if (!a || !b) return;
      const mx = (a.x + b.x) / 2;
      const sel = selected?.kind === "wire" && selected.id === w.id;
      const path = el("path", {d: `M${a.x} ${a.y} C${mx} ${a.y} ${mx} ${b.y} ${b.x} ${b.y}`, fill: "none",
        stroke: w.color, "stroke-width": sel ? 4 : 2.5, style: "cursor:pointer"}, g);
      path.addEventListener("pointerdown", (e) => { e.stopPropagation(); selected = {kind: "wire", id: w.id}; render(); });
    });
  }

  function clickPin(ref) {
    if (!pending) { pending = ref; return render(); }
    const same = pending.part === ref.part && pending.pin === ref.pin && pending.side === ref.side;
    if (!same) state.wires.push({id: uid++, from: pending, to: ref, color: $("wireColor").value});
    pending = null; render();
  }

  function toWorld(e) {
    const r = svg.getBoundingClientRect();
    return {x: (e.clientX - r.left - state.view.x) / state.view.k, y: (e.clientY - r.top - state.view.y) / state.view.k};
  }
  function startDrag(e, part) {
    e.stopPropagation(); selected = {kind: "part", id: part.id};
    const w0 = toWorld(e), ox = part.x, oy = part.y;
    const move = (ev) => { const w = toWorld(ev); part.x = ox + w.x - w0.x; part.y = oy + w.y - w0.y; render(); };
    const up = () => { document.removeEventListener("pointermove", move); document.removeEventListener("pointerup", up); };
    document.addEventListener("pointermove", move); document.addEventListener("pointerup", up); render();
  }
  svg.addEventListener("pointerdown", (e) => {
    selected = null; pending = null;
    const sx = e.clientX, sy = e.clientY, vx = state.view.x, vy = state.view.y;
    const move = (ev) => { state.view.x = vx + ev.clientX - sx; state.view.y = vy + ev.clientY - sy; render(); };
    const up = () => { document.removeEventListener("pointermove", move); document.removeEventListener("pointerup", up); };
    document.addEventListener("pointermove", move); document.addEventListener("pointerup", up); render();
  });
  svg.addEventListener("wheel", (e) => {
    e.preventDefault();
    const k0 = state.view.k, k = Math.min(3, Math.max(.3, k0 * (e.deltaY < 0 ? 1.1 : .9)));
    const r = svg.getBoundingClientRect(), mx = e.clientX - r.left, my = e.clientY - r.top;
    state.view.x = mx - (mx - state.view.x) * k / k0; state.view.y = my - (my - state.view.y) * k / k0; state.view.k = k; render();
  }, {passive: false});

  function deleteSelected() {
    if (!selected) return;
    if (selected.kind === "wire") state.wires = state.wires.filter(w => w.id !== selected.id);
    else { state.parts = state.parts.filter(p => p.id !== selected.id);
           state.wires = state.wires.filter(w => w.from.part !== selected.id && w.to.part !== selected.id); }
    selected = null; render();
  }
  document.addEventListener("keydown", (e) => {
    if (["INPUT", "TEXTAREA"].includes(document.activeElement.tagName)) return;
    if (e.key === "Delete" || e.key === "Backspace") deleteSelected();
  });
  $("btnDelete").onclick = deleteSelected;
  $("btnClear").onclick = () => { state.parts = []; state.wires = []; selected = pending = null; render(); };

  // палитра
  const pal = $("palette");
  Object.entries(DEFS).forEach(([type, d]) => {
    const b = document.createElement("button"); b.textContent = d.label;
    b.onclick = () => {
      const right = state.parts.reduce((m, p) => Math.max(m, p.x + DEFS[p.type].w), -Infinity);
      const top = state.parts.length ? Math.min(...state.parts.map(p => p.y)) : 30 - state.view.y / state.view.k;
      const x = state.parts.length ? right + 80 : 40 - state.view.x / state.view.k;   // место для проводов между деталями
      state.parts.push({id: uid++, type, x, y: top});
      render();
    };
    pal.appendChild(b);
  });

  // сохранение / загрузка
  const status = (m) => { $("status").textContent = m; };
  const api = (url, opt = {}) => fetch(url, {...opt, headers: {"Content-Type": "application/json", "X-CSRF-Token": B.csrf}});

  async function save() {
    if (!B.loggedIn) return status(B.needLogin);
    const title = $("designTitle").value.trim();
    if (!title) return $("designTitle").focus();
    const r = await api("/api/designs", {method: "POST", body: JSON.stringify({
      id: mine ? currentId : null, title, public: $("designPublic").checked, data: {...state, uid}})});
    if (r.ok) { currentId = (await r.json()).id; mine = true; status(B.savedLabel); loadList(); } else status("Error " + r.status);
  }
  $("btnSave").onclick = save;

  async function loadDesign(id) {
    const r = await api("/api/designs/" + id); if (!r.ok) return;
    const d = await r.json();
    state = d.data; uid = state.uid || 1000; currentId = d.id; mine = d.mine;
    $("designTitle").value = d.title; $("designPublic").checked = !!d.public; render();
  }
  async function loadList() {
    const ul = $("myDesigns"); ul.innerHTML = "";
    if (!B.loggedIn) return;
    const r = await api("/api/designs"); if (!r.ok) return;
    (await r.json()).forEach(d => {
      const li = document.createElement("li");
      const a = document.createElement("a"); a.href = "#"; a.textContent = d.title;
      a.onclick = (e) => { e.preventDefault(); loadDesign(d.id); };
      const x = document.createElement("button"); x.className = "link"; x.textContent = "✕";
      x.onclick = async () => { await api("/api/designs/" + d.id, {method: "DELETE"}); loadList(); };
      li.append(a, x); ul.appendChild(li);
    });
  }
  render(); loadList();
  if (currentId) loadDesign(currentId);
})();
