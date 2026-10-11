document.addEventListener("click", async (e) => {
  const copy = e.target.closest("[data-copy]");
  if (copy) {
    const text = document.querySelector(copy.dataset.copy).value;
    try { await navigator.clipboard.writeText(text); const old = copy.textContent; copy.textContent = "✓"; setTimeout(() => copy.textContent = old, 1200); } catch (_) {}
  }
  if (e.target.closest("[data-share]")) {
    const url = location.href.split("#")[0];
    if (navigator.share) { try { await navigator.share({url}); return; } catch (_) {} }
    try { await navigator.clipboard.writeText(url); e.target.textContent = "✓"; } catch (_) {}
  }
});
