/*
 * Omega in the browser: draws the text screen port/be_web.c sends
 * (Module.om): one canvas, the 16 PC colours of Omega's MSDOS build.
 * Keyboard, saves in IndexedDB (IDBFS, /save). Loaded before omega-core.js.
 * Structure copied from ~/Games/roguepc/web/roguepc.js.
 */
(function () {
	'use strict';

	var DIR = '/save', SAVE = DIR + '/omega.sav';
	var FONT = '"DejaVu Sans Mono", Menlo, Consolas, "Liberation Mono", monospace';
	var PAL = ['#000000', '#0000aa', '#00aa00', '#00aaaa', '#aa0000', '#aa00aa', '#aa5500', '#aaaaaa',
		'#555555', '#5555ff', '#55ff55', '#55ffff', '#ff5555', '#ff55ff', '#ffff55', '#ffffff'];
	var A_COLOR = 0x7f00, A_STANDOUT = 0x10000, A_TILE = 0x20000, MAPW = 64;
	/* map tiles: Kinder's sheet; C picks the tile (port/tiles.c, bits 18+), square cells of row height,
	 * scrolled sideways to keep the player (cursor) in view, like WinOmega */
	var tilesOn = true, sheet = new Image(), tox = 0;
	try { tilesOn = localStorage.getItem('omega-tiles') !== '0'; } catch (e) { }
	sheet.onload = function () { dirty = true; draw(); };
	sheet.src = 'tiles.png';
	/* arrows and keypad = Omega's number keys (moving, and 8/2 in lists) */
	var KEYS = { ArrowUp: 56, ArrowDown: 50, ArrowLeft: 52, ArrowRight: 54, Home: 55, PageUp: 57,
		End: 49, PageDown: 51, Clear: 53, Enter: 10, Escape: 27, Backspace: 8, Delete: 8, Tab: 9 };

	var events = [], running = false, lastSave = 0;
	var cols = 80, rows = 24, scr = null, cur = { y: 0, x: 0 }, hero = { y: 0, x: 0 };
	var auto = true, cv, ctx, wm = null, rects = {}, LAYOUT = '/save/web-layout.json', L = { px: 0, font: 13, wm: null }, px = 18, cw = 11, ch = 22, dirty = true;
	var dpr = Math.max(1, Math.min(3, window.devicePixelRatio || 1));

	/* message history: lines the game sends (be_msg), append 1 = run-on text,
	 * 2 = the game folded a repeat: replace the last line */
	function msg(s, append) {
		var l = $('log'), d = l.lastChild;
		if (append !== 1 || !d) return RvipWM.log(l, s, append === 2);
		var end = l.scrollTop + l.clientHeight >= l.scrollHeight - 4;
		d.dataset.s += s; d.textContent += s;
		if (end) l.scrollTop = l.scrollHeight;
	}
	function $(id) { return document.getElementById(id); }
	function status(msg, isError) {
		var s = $('status');
		s.textContent = msg; s.hidden = !msg; s.classList.toggle('error', !!isError);
	}

	/* ---------- drawing: the whole screen (cv) and one canvas per pane, all from the game's cells ---------- */
	var MAP = 1, SIDE = 2, STAT = 3, MSG = 4, PANE_BOX = { 1: 'map', 2: 'side', 3: 'stat', 4: 'msgcv' }, P = {}, popup = true, msgT = '';
	function measure() {
		ctx.font = px + 'px ' + FONT;
		cw = Math.ceil(ctx.measureText('M').width); ch = Math.ceil(px * 1.2);
		size(cv, cols * cw, rows * ch);
		dirty = true;
	}
	function size(c, w, h) {
		if (c.width !== w * dpr || c.height !== h * dpr) { c.width = w * dpr; c.height = h * dpr; }
		c.style.width = w + 'px'; c.style.height = h + 'px';
		var g = c.getContext('2d');
		g.setTransform(dpr, 0, 0, dpr, 0, 0); g.font = px + 'px ' + FONT; g.textBaseline = 'top';
		return g;
	}
	/* biggest font that shows the map (single window: the whole screen) in the map window */
	function fit() {
		var b = $('map'), best = 8, one = !rects.side && !rects.stat, w = one ? cols : (P[MAP] ? P[MAP].c : 64), h = one ? rows : (P[MAP] ? P[MAP].r : rows - 6);
		for (var p = 8; p <= 40; p++) {
			ctx.font = p + 'px ' + FONT;
			if (Math.ceil(ctx.measureText('M').width) * w <= b.clientWidth && Math.ceil(p * 1.2) * h <= b.clientHeight) best = p;
		}
		return best;
	}
	/* the map camera (RVIP.md W4): (fx, fy) centred, clamped at the edges */
	function scroll(c, fx, fy) { RvipWM.center(c, fx, fy, parseFloat(c.style.width), parseFloat(c.style.height)); }
	function cell(g, v, x, y, w) {
		var c = v & 0xff, fg = v >> 8 & 15, bg = v >> 12 & 7, t;
		if (!(v & A_COLOR)) fg = 7;
		if (v & A_STANDOUT) { t = fg; fg = bg; bg = t; }
		if (bg) { g.fillStyle = PAL[bg]; g.fillRect(x, y, w, ch); }
		if (c > 32) { g.fillStyle = PAL[fg]; g.fillText(String.fromCharCode(c), x + (w - cw) / 2, y + (ch - px) / 2); }
	}
	function drawPane(p) {
		var q = P[p], c = $(PANE_BOX[p]).firstChild, w = p === MAP && tilesOn ? ch : cw;
		if (!q || !c) return;
		var g = size(c, q.c * w, q.r * ch);
		g.fillStyle = '#000'; g.fillRect(0, 0, q.c * w, q.r * ch);
		g.imageSmoothingEnabled = false;
		for (var y = 0; y < q.r; y++)
			for (var x = 0; x < q.c; x++) {
				var v = q.buf[y * q.c + x], t = v >>> 18;
				if (p === MAP && tilesOn && t-- && sheet.complete && sheet.naturalWidth) g.drawImage(sheet, t % 128 * 32, (t >> 7) * 32, 32, 32, x * w, y * ch, w, ch);
				else cell(g, v, x * w, y * ch, w);
			}
		if (p === MSG) {   /* shown while the game writes to it; a keypress hides it until the line changes */
			var t = String.fromCharCode.apply(null, q.buf.map(function (v) { return v & 0xff || 32; })).trim();
			if (t !== msgT) { msgT = t; c.parentNode.hidden = !t; }
		}
		if (p !== MAP) return;
		var cy = cur.y - q.y, cx = cur.x - q.x;
		if (cy >= 0 && cy < q.r && cx >= 0 && cx < q.c) {
			g.strokeStyle = PAL[14]; g.lineWidth = 1; g.strokeRect(cx * w + 0.5, cy * ch + 0.5, w - 1, ch - 1);
		}
		scroll(c, (hero.x - q.x + 0.5) * w, (hero.y - q.y + 0.5) * ch);
	}
	function saveLayout() { try { Module.FS.writeFile(LAYOUT, JSON.stringify(L)); syncFiles(); } catch (e) { } }
	function fonts() { ['log', 'inv', 'vis'].forEach(function (id) { $(id).style.fontSize = L.font + 'px'; }); }
	/* the shared tiling window manager (rvip-wm.js, RVIP.md 5b) */
	function makeWM() {
		try { var s = JSON.parse(Module.FS.readFile(LAYOUT, { encoding: 'utf8' })); if (s) L = { px: s.px | 0, font: s.font || 13, wm: s.wm }; } catch (e) { }
		if (L.px >= 8 && L.px <= 40) { px = L.px; auto = false; measure(); }
		fonts();
		wm = RvipWM({
			area: $('game'), menu: $('btn-layout'),
			wins: [{ id: 'map', title: 'Map' }, { id: 'side', title: 'Side panel' }, { id: 'stat', title: 'Status' },
				{ id: 'msg', title: 'Messages' }, { id: 'inv', title: 'Inventory' }, { id: 'vis', title: 'Visible' }],
			multi: { d: 'h', r: 0.68, a: { d: 'v', r: 0.84, a: { d: 'h', r: 0.82, a: 'map', b: 'side' }, b: 'stat' },
				b: { d: 'v', r: 0.35, a: 'msg', b: { d: 'v', r: 0.6, a: 'inv', b: 'vis' } } },
			single: 'map',
			state: L.wm, noFont: 'map',
			save: function (st) { L.wm = st; saveLayout(); },
			layout: function (r) { rects = r; if (auto) { px = fit(); measure(); } dirty = true; draw(); },
			font: function (id, d) { L.font = Math.max(8, Math.min(28, L.font + d)); fonts(); saveLayout(); },
			onReset: function () { auto = true; L.px = 0; L.font = 13; L.wm = wm.state(); fonts(); px = fit(); measure(); draw(); saveLayout(); }
		});
		wm.apply();
	}
	/* single window: the whole screen in the map window; multi: the panes, the whole screen over them while a pop-up is up */
	function draw() {
		if (!dirty || !scr || !wm) return;
		dirty = false;
		var one = !rects.side && !rects.stat, box = one ? $('map') : $('full');
		if (cv.parentNode !== box) box.appendChild(cv);
		$('map').firstChild.style.display = one ? 'none' : '';
		$('full').hidden = one || !popup;
		if (one || popup) drawFull(one);
		if (!one) [MAP, SIDE, STAT, MSG].forEach(drawPane);
	}
	function drawFull(one) {
		ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
		ctx.fillStyle = '#000'; ctx.fillRect(0, 0, cols * cw, rows * ch);
		for (var y = 0; y < rows; y++)
			for (var x = 0; x < cols; x++)
				if (!(tilesOn && x < MAPW && scr[y * cols + x] & A_TILE)) cell(ctx, scr[y * cols + x], x * cw, y * ch, cw);
		if (!(tilesOn && drawTiles())) { ctx.fillStyle = PAL[7]; ctx.fillRect(cur.x * cw, cur.y * ch + ch - 2, cw, 2); }
		if (one) { size(cv, cols * cw, rows * ch); return scroll(cv, (hero.x + 0.5) * cw, (hero.y + 0.5) * ch); }
		/* pop-up over the panes: the whole screen, scaled down to fit */
		var b = $('full'), s = Math.min(1, b.clientWidth / (cols * cw), b.clientHeight / (rows * ch));
		cv.style.width = cols * cw * s + 'px'; cv.style.height = rows * ch * s + 'px';
		scroll(cv, 0, 0);
	}
	/* true when the cursor is on the map (drawn here as a box) */
	function drawTiles() {
		var T = ch, nx = Math.min(MAPW, Math.floor(MAPW * cw / T)), onMap = cur.x < MAPW && scr[cur.y * cols] & A_TILE;
		tox = Math.max(0, Math.min(MAPW - nx, hero.x - (nx >> 1)));
		ctx.imageSmoothingEnabled = false;
		for (var y = 0; y < rows; y++) {
			if (!(scr[y * cols] & A_TILE)) continue;
			for (var i = 0; i < nx; i++) {
				var v = scr[y * cols + tox + i], t = v >>> 18;
				if (t-- && sheet.complete && sheet.naturalWidth) ctx.drawImage(sheet, t % 128 * 32, (t >> 7) * 32, 32, 32, i * T, y * ch, T, T);
				else cell(ctx, v, i * T, y * ch, T);
			}
		}
		if (!onMap) return false;
		ctx.strokeStyle = PAL[14]; ctx.lineWidth = 1;
		ctx.strokeRect((cur.x - tox) * T + 0.5, cur.y * ch + 0.5, T - 1, T - 1);
		return true;
	}
	function toggleTiles() {
		tilesOn = !tilesOn;
		try { localStorage.setItem('omega-tiles', tilesOn ? '1' : '0'); } catch (e) { }
		$('btn-tiles').classList.toggle('on', tilesOn);
		dirty = true; draw();
	}
	function zoom(d) {
		auto = false;
		px = Math.max(8, Math.min(40, px + d));
		measure(); draw();
		L.px = px; saveLayout();
	}

	var om = {
		init: function (c, r) {
			cols = c; rows = r; scr = new Uint32Array(c * r);
			$('game').hidden = false;
			px = 16; measure(); makeWM();
		},
		put: function (y, x, v) { scr[y * cols + x] = v; dirty = true; },
		cursor: function (y, x) { cur.y = y; cur.x = x; dirty = true; },
		hero: function (y, x) { hero.y = y; hero.x = x; dirty = true; },
		pane: function (p, y, x, r, c) { P[p] = { y: y, x: x, r: r, c: c, buf: new Uint32Array(r * c) }; dirty = true; },
		pput: function (p, y, x, v) { P[p].buf[y * P[p].c + x] = v; dirty = true; },
		popup: function (on) { if (popup !== !!on) { popup = !!on; dirty = true; } },
		msg: msg,
		flush: function () { draw(); },
		lists: function (inv, vis) {
			$('inv').innerHTML = '';
			inv.split('\n').forEach(function (l) {
				if (!l) return;
				var t = l.split('\t'), d = document.createElement('div');
				d.textContent = t[1]; d.style.color = PAL[+t[0] || 7]; $('inv').appendChild(d);
			});
			RvipWM.visible($('vis'), vis.replace(/\t(\d+)$/gm, function (m, c) { return '\t' + PAL[+c || 7]; }));
		},
		key: function () { return events.length ? events.shift() : -1; },
		/* autosave at most every 2 s, and when the page is hidden */
		wantSave: function () {
			var now = performance.now();
			if (now - lastSave < 2000 && !document.hidden) return 0;
			if (!wantSaveFlag) return 0;
			wantSaveFlag = false; lastSave = now;
			setTimeout(syncFiles, 0);
			return 1;
		},
		end: function (saved) {
			running = false;
			syncFiles(function () {
				$('overlay-msg').textContent = saved ? 'Your game has been saved. Play again to continue it.' : 'The game is over.';
				$('overlay').hidden = false;
			});
		}
	};
	var wantSaveFlag = false;   /* a key was pressed since the last autosave */

	/* ---------- input ---------- */
	function onKey(e) {
		if (!$('help').hidden) {
			if (e.key === 'Escape') { $('help').hidden = true; e.preventDefault(); }
			return;
		}
		if (!running || e.isComposing || e.metaKey) return;
		var k = e.key, code = e.code || '', m = /^Numpad(\d)$/.exec(code), c;
		if (m) c = 48 + +m[1];
		else if (code === 'NumpadEnter') c = 10;
		else if (code === 'NumpadDecimal') c = 46;
		else if (KEYS[k] !== undefined) c = KEYS[k];
		else if (k.length === 1) {
			c = k.charCodeAt(0);
			if (e.ctrlKey && !e.altKey) {
				var u = k.toUpperCase().charCodeAt(0);
				if (u >= 65 && u <= 90) c = u & 0x1f; else return;
			}
			if (c > 126) return;
		}
		else return;
		events.push(c);
		wantSaveFlag = true;
		e.preventDefault();
	}

	/* ---------- saves: IndexedDB (IDBFS) ---------- */
	var syncing = false, syncAgain = false, pendingCbs = [];
	function syncFiles(cb) {
		if (!Module.FS) { if (cb) cb(); return; }
		if (typeof cb === 'function') pendingCbs.push(cb);
		if (syncing) { syncAgain = true; return; }
		syncing = true;
		var cbs = pendingCbs; pendingCbs = [];
		Module.FS.syncfs(false, function (err) {
			syncing = false;
			if (err) status('Saving to browser storage (IndexedDB) failed: ' + err + '. Use "Export save" to keep a copy.', true);
			cbs.forEach(function (f) { f(err); });
			if (syncAgain) { syncAgain = false; syncFiles(); }
		});
	}
	function hasSave() { try { Module.FS.stat(SAVE); return true; } catch (e) { return false; } }
	function exportSave() {
		if (!hasSave()) { status('There is no saved game yet.', true); setTimeout(function () { status(''); }, 2000); return; }
		var a = document.createElement('a');
		a.href = URL.createObjectURL(new Blob([Module.FS.readFile(SAVE)], { type: 'application/octet-stream' }));
		a.download = 'omega.sav';
		document.body.appendChild(a); a.click();
		setTimeout(function () { URL.revokeObjectURL(a.href); a.remove(); }, 1000);
	}
	function importSave(file) {
		var r = new FileReader();
		r.onload = function () {
			if (!confirm('Replace the current game with "' + file.name + '"?')) return;
			running = false;
			Module.FS.writeFile(SAVE, new Uint8Array(r.result));
			syncFiles(function (err) { if (!err) location.reload(); });
		};
		r.readAsArrayBuffer(file);
	}
	function newGame() {
		if (!confirm('Delete the saved game in this browser and start a new one?')) return;
		running = false;
		if (hasSave()) Module.FS.unlink(SAVE);
		syncFiles(function (err) { if (!err) location.reload(); });
	}

	/* ---------- help ---------- */
	var helpLoaded = false;
	function toggleHelp() {
		var h = $('help');
		h.hidden = !h.hidden;
		if (!h.hidden && !helpLoaded) {
			helpLoaded = true;
			fetch('help.html').then(function (r) { if (!r.ok) throw new Error(r.status); return r.text(); })
				.then(function (t) { $('help-body').innerHTML = t; })
				.catch(function (err) { helpLoaded = false; $('help-body').textContent = 'Could not load the guide (' + err + '). Press ? in the game for its own help.'; });
		}
		if (!h.hidden) $('help-body').focus();
	}

	/* ---------- startup ---------- */
	window.Module = {
		om: om,
		arguments: [],
		preRun: [function () {
			var FS = Module.FS;
			Module.ENV.OMEGALIB = '/omegalib/';
			Module.ENV.HOME = DIR;
			Module.ENV.OMEGA_LINES = '40';
			FS.mkdirTree(DIR);
			FS.mount(Module.IDBFS, {}, DIR);
			FS.chdir(DIR);
			Module.addRunDependency('idbfs');
			FS.syncfs(true, function (err) {
				if (err) status('Could not read saved games from IndexedDB (' + err + '). Saving may not work in this browser mode.', true);
				if (hasSave()) Module.arguments.push('omega.sav');
				Module.removeRunDependency('idbfs');
			});
		}],
		onRuntimeInitialized: function () { running = true; status(''); },
		print: function (s) { console.log(s); },
		printErr: function (s) { console.warn(s); },
		setStatus: function (s) { if (s && !running) status(s.replace(/\(\d+\/\d+\)/, '').trim() || 'Loading…'); },
		onAbort: function (what) { crashed(what); }
	};
	function crashed(err) {
		if (!running) return;
		running = false;
		var msg = (err && (err.message || err.reason && err.reason.message)) || String(err);
		console.error('[omega] crash:', err);
		status('The game crashed (' + msg + '). Reload the page to continue from the last autosave.', true);
	}
	window.addEventListener('unhandledrejection', function (e) {
		if (e.reason && e.reason.name === 'ExitStatus') return;   /* exit() is the normal end */
		crashed(e.reason);
	});
	window.addEventListener('error', function (e) {
		if (e.error && e.error.name === 'ExitStatus') return;
		if (e.error instanceof WebAssembly.RuntimeError || /omega-core/.test(e.filename || '')) crashed(e.error || e.message);
	});
	document.addEventListener('visibilitychange', function () { if (document.hidden) wantSaveFlag = true; });

	window.addEventListener('resize', function () { if (wm) wm.apply(); });
	document.addEventListener('keydown', onKey);
	document.addEventListener('keydown', function () { $('msgcv').hidden = true; });
	document.addEventListener('DOMContentLoaded', function () {
		cv = document.createElement('canvas');
		ctx = cv.getContext('2d');
		$('btn-export').onclick = exportSave;
		$('btn-import').onclick = function () { $('import-file').click(); };
		$('import-file').onchange = function () { if (this.files[0]) importSave(this.files[0]); this.value = ''; };
		$('btn-new').onclick = newGame;
		$('btn-help').onclick = toggleHelp;
		$('btn-tiles').onclick = toggleTiles;
		$('btn-tiles').classList.toggle('on', tilesOn);
		$('help-close').onclick = toggleHelp;
		$('btn-zoom-in').onclick = function () { zoom(1); };
		$('btn-zoom-out').onclick = function () { zoom(-1); };
		$('btn-restart').onclick = function () { location.reload(); };
		document.querySelectorAll('button').forEach(function (b) {
			b.addEventListener('mousedown', function (e) { e.preventDefault(); });
		});
	});
})();
