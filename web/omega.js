/*
 * Omega in the browser: draws the text screen port/be_web.c sends
 * (Module.om): one canvas, the 16 PC colours of Omega's MSDOS build.
 * Keyboard, saves in IndexedDB (IDBFS, /save). Loaded before omega-core.js.
 * Structure copied from ~/Games/roguepc/web/roguepc.js.
 */
(function () {
	'use strict';

	var DIR = RvipApp.dir, SAVE = DIR + '/omega.sav';
	var FONT = '"DejaVu Sans Mono", Menlo, Consolas, "Liberation Mono", monospace';
	var PAL = ['#000000', '#0000aa', '#00aa00', '#00aaaa', '#aa0000', '#aa00aa', '#aa5500', '#aaaaaa',
		'#555555', '#5555ff', '#55ff55', '#55ffff', '#ff5555', '#ff55ff', '#ffff55', '#ffffff'];
	var A_COLOR = 0x7f00, A_STANDOUT = 0x10000, A_TILE = 0x20000, MAPW = 64;
	/* map tiles: Kinder's sheet; C picks the tile (port/tiles.c, bits 18+), square cells of row height,
	 * scrolled sideways to keep the player (cursor) in view, like WinOmega */
	var tilesOn = true, sheet = new Image(), tox = 0;
	sheet.onload = function () { dirty = true; draw(); if (tilesOn) renderLists(true); };
	sheet.src = 'tiles.png';
	/* arrows and keypad = Omega's number keys (moving, and 8/2 in lists) */
	var KEYS = { ArrowUp: 56, ArrowDown: 50, ArrowLeft: 52, ArrowRight: 54, Home: 55, PageUp: 57,
		End: 49, PageDown: 51, Clear: 53, Enter: 10, Escape: 27, Backspace: 8, Delete: 8, Tab: 9 };

	var events = [], lastSave = 0, app;
	var cols = 80, rows = 24, scr = null, cur = { y: 0, x: 0 }, hero = { y: 0, x: 0 };
	var cv, ctx, wm = null, rects = {}, LAYOUT = DIR + '/web-layout.json', L = { wm: null }, px = 18, cw = 11, ch = 22, dirty = true;
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
	function status(msg, isError) { app.status(msg, isError); }

	/* ---------- drawing: the whole screen (cv) and one canvas per pane, all from the game's cells ---------- */
	var MAP = 1, SIDE = 2, STAT = 3, MSG = 4, PANE_BOX = { 1: 'map', 2: 'side', 3: 'stat' }, P = {}, popup = true;
	/* fonts: L.face for the text windows and pop-ups, L.mapFace for the map (text mode) */
	function face(map) { var n = map ? L.mapFace : L.face; return n ? '"' + n + '", ' + FONT : FONT; }
	function measure() {
		ctx.font = px + 'px ' + face(true);
		cw = Math.ceil(ctx.measureText('M').width); ch = Math.ceil(px * 1.2);
		size(cv, cols * cw, rows * ch);
		dirty = true;
	}
	/* cell metrics of a pane: the map follows the zoom (px), Side panel and Status their own A−/A+ size */
	function met(p) {
		if (p === MAP) return { px: px, cw: cw, ch: ch };
		var f = RvipWM.fontSize(PANE_BOX[p]);
		ctx.font = f + 'px ' + face(false);
		return { px: f, cw: Math.ceil(ctx.measureText('M').width), ch: Math.ceil(f * 1.2) };
	}
	function size(c, w, h, map, f) {
		if (c.width !== w * dpr || c.height !== h * dpr) { c.width = w * dpr; c.height = h * dpr; }
		c.style.width = w + 'px'; c.style.height = h + 'px';
		var g = c.getContext('2d');
		g.setTransform(dpr, 0, 0, dpr, 0, 0); g.font = (f || px) + 'px ' + face(map); g.textBaseline = 'top';
		return g;
	}
	/* biggest font that shows the map (single window: the whole screen) in the map window */
	function fit() {
		var b = $('map'), best = 8, one = !rects.side && !rects.stat, w = one ? cols : (P[MAP] ? P[MAP].c : 64), h = one ? rows : (P[MAP] ? P[MAP].r : rows - 6);
		for (var p = 8; p <= 40; p++) {
			ctx.font = p + 'px ' + face(true);
			if (Math.ceil(ctx.measureText('M').width) * w <= b.clientWidth && Math.ceil(p * 1.2) * h <= b.clientHeight) best = p;
		}
		return best;
	}
	/* the map camera (RVIP.md W4): (fx, fy) centred, clamped at the edges */
	function scroll(c, fx, fy) { RvipWM.center(c, fx, fy, parseFloat(c.style.width), parseFloat(c.style.height)); }
	function cell(g, v, x, y, w, m) {
		m = m || { px: px, cw: cw, ch: ch };
		var c = v & 0xff, fg = v >> 8 & 15, bg = v >> 12 & 7, t;
		if (!(v & A_COLOR)) fg = 7;
		if (v & A_STANDOUT) { t = fg; fg = bg; bg = t; }
		if (bg) { g.fillStyle = PAL[bg]; g.fillRect(x, y, w, m.ch); }
		if (c > 32) { g.fillStyle = PAL[fg]; g.fillText(String.fromCharCode(c), x + (w - m.cw) / 2, y + (m.ch - m.px) / 2); }
	}
	function paneText(p) {
		var q = P[p];
		if (!q) return '';
		var t = String.fromCharCode.apply(null, q.buf.map(function (v) { return v & 0xff || 32; }));
		return t.replace(new RegExp('.{' + q.c + '}', 'g'), '$&\n').replace(/ +$/gm, '').trim();
	}
	function drawPane(p) {
		var q = P[p], c = $(PANE_BOX[p]).firstChild;
		if (!q || !c) return;
		var m = met(p), ch = m.ch, w = p === MAP && tilesOn ? ch : m.cw;
		var g = size(c, q.c * w, q.r * ch, p === MAP, m.px);
		g.fillStyle = '#000'; g.fillRect(0, 0, q.c * w, q.r * ch);
		g.imageSmoothingEnabled = false;
		for (var y = 0; y < q.r; y++)
			for (var x = 0; x < q.c; x++) {
				var v = q.buf[y * q.c + x], t = v >>> 18;
				if (p === MAP && tilesOn && t-- && sheet.complete && sheet.naturalWidth) g.drawImage(sheet, t % 128 * 32, (t >> 7) * 32, 32, 32, x * w, y * ch, w, ch);
				else cell(g, v, x * w, y * ch, w, m);
			}
		if (p !== MAP) return;
		var cy = cur.y - q.y, cx = cur.x - q.x;
		if (cy >= 0 && cy < q.r && cx >= 0 && cx < q.c && !(cur.y === hero.y && cur.x === hero.x)) {
			g.strokeStyle = PAL[14]; g.lineWidth = 1; g.strokeRect(cx * w + 0.5, cy * ch + 0.5, w - 1, ch - 1);
		}
		scroll(c, (hero.x - q.x + 0.5) * w, (hero.y - q.y + 0.5) * ch);
	}
	function saveLayout() { try { Module.FS.writeFile(LAYOUT, JSON.stringify(L)); app.sync(); } catch (e) { } }
	function fonts() {
		['log', 'inv', 'vis'].forEach(function (id) { $(id).style.fontFamily = L.face ? '"' + L.face + '", monospace' : ''; });
	}
	/* a face from the index page's fonts/ (web/build.sh lists them in fonts.json) */
	function loadFace(n, now) {
		var redraw = function () { fonts(); measure(); draw(); };
		if (!n) { if (now) redraw(); return; }
		var ff = new FontFace(n, 'url(../fonts/' + n + '.woff)');
		ff.load().then(function () { document.fonts.add(ff); redraw(); }).catch(function () { status('Could not load the font ' + n + '.', true); });
	}
	/* map font chooser: on the Map title bar (shown on hover), text mode only */
	var mapSel = document.createElement('select');
	mapSel.title = 'Map font (text mode)';
	mapSel.innerHTML = '<option value="">Default font</option>';
	mapSel.addEventListener('pointerdown', function (e) { e.stopPropagation(); });
	function renderMapSel() {
		var bs = document.querySelector('#t-map .wm-btns');
		if (bs && mapSel.parentNode !== bs) bs.insertBefore(mapSel, bs.firstChild);
		mapSel.hidden = tilesOn;
		mapSel.value = L.mapFace || '';
	}
	/* the shared tiling window manager (rvip-wm.js, RVIP.md 5b) */
	function makeWM() {
		try { var s = JSON.parse(Module.FS.readFile(LAYOUT, { encoding: 'utf8' })); if (s) { L = { wm: s.wm, face: typeof s.face === 'string' ? s.face : '', mapFace: typeof s.mapFace === 'string' ? s.mapFace : '' };
			var old = s.fs || (s.font && { msg: s.font, inv: s.font, vis: s.font });   /* old layout: sizes move to the WM */
			if (old && L.wm && !L.wm.fs) L.wm.fs = old;
			if (s.px >= 8 && L.wm && L.wm.fs && !L.wm.fs.map) L.wm.fs.map = s.px; } } catch (e) { }   /* old map zoom: the WM's (multi-window) */
		$('sel-font').value = L.face || '';
		loadFace(L.face); loadFace(L.mapFace);
		fonts();
		wm = RvipWM({
			area: $('game'), menu: $('btn-layout'),
			wins: [{ id: 'map', title: 'Map' }, { id: 'side', title: 'Side panel' }, { id: 'stat', title: 'Status' },
				{ id: 'msg', title: 'Messages' }, { id: 'inv', title: 'Inventory' }, { id: 'vis', title: 'Visible' }],
			multi: { d: 'h', r: 0.68, a: { d: 'v', r: 0.84, a: { d: 'h', r: 0.82, a: 'map', b: 'side' }, b: 'stat' },
				b: { d: 'v', r: 0.35, a: 'msg', b: { d: 'v', r: 0.6, a: 'inv', b: 'vis' } } },
			single: 'map',
			state: L.wm,
			save: function (st) { L.wm = st; saveLayout(); },
			/* the map zoom is the WM's, one per mode (RVIP.md W4); none set: fit the window */
			layout: function (r) { rects = r; renderMapSel(); px = (wm.zoomed && wm.zoomed('map')) || fit();   /* wm.zoomed: rvip-wm.js from 2026-09-28 */ measure(); draw(); },
			zoom: { map: function (s) { px = s; measure(); draw(); }, side: redraw, stat: redraw },   /* Side panel, Status: canvases at their own size */
			size: { map: function () { return px; } },   /* A+ / A− step from the drawn size */
			fontMax: { map: 40 },
			onReset: function () { L.wm = wm.state(); fonts(); saveLayout(); }
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
		if (!one) { [MAP, SIDE, STAT].forEach(drawPane); RvipWM.prompt.text(paneText(MSG)); }
	}
	function drawFull(one) {
		ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
		ctx.font = px + 'px ' + face(one);
		ctx.fillStyle = '#000'; ctx.fillRect(0, 0, cols * cw, rows * ch);
		/* map tiles first, then every text cell (side panel, messages, menus
		   and lists over the map) at its text position, on black */
		var onMap = tilesOn && drawTiles();
		for (var y = 0; y < rows; y++)
			for (var x = 0; x < cols; x++) {
				var v = scr[y * cols + x];
				if (tilesOn && x < MAPW && v & A_TILE) continue;
				if (tilesOn) { ctx.fillStyle = '#000'; ctx.fillRect(x * cw, y * ch, cw, ch); }
				cell(ctx, v, x * cw, y * ch, cw);
			}
		if (!onMap && (cur.y !== hero.y || cur.x !== hero.x)) { ctx.fillStyle = PAL[7]; ctx.fillRect(cur.x * cw, cur.y * ch + ch - 2, cw, 2); }
		/* the camera on the hero, where it is drawn: a tile (ch wide, scrolled by tox) or a text cell */
		var ht = tilesOn && hero.x < MAPW && scr[hero.y * cols + hero.x] & A_TILE;
		if (one) { size(cv, cols * cw, rows * ch, true); return scroll(cv, ht ? (hero.x - tox + 0.5) * ch : (hero.x + 0.5) * cw, (hero.y + 0.5) * ch); }
		/* pop-up over the panes: the whole screen, scaled down to fit */
		var b = $('full'), s = Math.min(1, b.clientWidth / (cols * cw), b.clientHeight / (rows * ch));
		cv.style.width = cols * cw * s + 'px'; cv.style.height = rows * ch * s + 'px';
		scroll(cv, 0, 0);
	}
	/* true when the cursor is on the map (drawn here as a box) */
	function drawTiles() {
		var T = ch, nx = Math.min(MAPW, Math.floor(MAPW * cw / T)), onMap = cur.x < MAPW && scr[cur.y * cols + cur.x] & A_TILE;
		tox = Math.max(0, Math.min(MAPW - nx, hero.x - (nx >> 1)));
		ctx.imageSmoothingEnabled = false;
		for (var y = 0; y < rows; y++)
			for (var i = 0; i < nx; i++) {
				var v = scr[y * cols + tox + i], t = v >>> 18;
				if (!(v & A_TILE)) continue;   /* text over the map: drawFull() */
				if (t-- && sheet.complete && sheet.naturalWidth) ctx.drawImage(sheet, t % 128 * 32, (t >> 7) * 32, 32, 32, i * T, y * ch, T, T);
				else cell(ctx, v, i * T, y * ch, T);
			}
		if (!onMap) return false;
		if (cur.y === hero.y && cur.x === hero.x) return true;   /* no cursor on the hero */
		ctx.strokeStyle = PAL[14]; ctx.lineWidth = 1;
		ctx.strokeRect((cur.x - tox) * T + 0.5, cur.y * ch + 0.5, T - 1, T - 1);
		return true;
	}
	/* the set's name (David Kinder's WinOmega sheet), None = text */
	function renderTilesBtn() { $('btn-tiles').textContent = 'Tiles: ' + (tilesOn ? 'WinOmega' : 'None'); }
	function toggleTiles() {
		tilesOn = !tilesOn;
		try { Module.FS.writeFile(DIR + '/web-tiles', tilesOn ? 'WinOmega' : 'None'); app.sync(); } catch (e) { }   /* IndexedDB */
		renderTilesBtn();
		renderMapSel();
		renderLists(true);
		dirty = true; draw();
	}
	function redraw() { dirty = true; draw(); }

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
		flush: function () { var l = $('log'); l.scrollTop = l.scrollHeight; draw(); },
		lists: function (inv, vis) { lastInv = inv; lastVis = vis; renderLists(); },
		/* atCmd: the game waits for a command, not a y/n or item prompt */
		key: function (atCmd) { RvipWM.prompt.wait(atCmd); return events.length ? events.shift() : -1; },
		/* autosave at most every 2 s, and when the page is hidden */
		wantSave: function () {
			var now = performance.now();
			if (now - lastSave < 2000 && !document.hidden) return 0;
			if (!wantSaveFlag) return 0;
			wantSaveFlag = false; lastSave = now;
			setTimeout(app.sync, 0);
			return 1;
		},
		end: function (saved) {
			app.running = false;
			app.sync(function () {
				$('overlay-msg').textContent = saved ? 'Your game has been saved. Play again to continue it.' : 'The game is over.';
				$('overlay').hidden = false;
			});
		}
	};
	var wantSaveFlag = false;   /* a key was pressed since the last autosave */
	/* Inventory and Visible windows from the lines be_web.c sends (tile and glyph
	 * chosen there): in tile mode the tile as an icon, in text mode the glyph */
	var lastInv = '', lastVis = '';
	function icon(t) {
		if (!tilesOn || !(t >= 0) || !sheet.naturalWidth) return null;
		var i = document.createElement('i');
		i.className = 'wm-ic';
		i.style.cssText = 'image-rendering:pixelated;background:url(' + sheet.src + ') -' + (t % 128) * 16 + 'px -' + (t >> 7) * 16 + 'px/' + sheet.naturalWidth / 2 + 'px auto';
		return i;
	}
	function renderLists(force) {
		var box = $('inv');
		box.innerHTML = '';
		lastInv.split('\n').forEach(function (l) {
			if (!l) return;
			var t = l.split('\t'), d = document.createElement('div'), ic = icon(+t[3]), b;
			d.style.color = PAL[+t[0] || 7];
			if (!ic) { ic = document.createElement('b'); ic.textContent = t[1]; ic.className = 'glyph'; }
			b = document.createElement('span'); b.textContent = t[2];
			d.appendChild(ic); d.appendChild(b); box.appendChild(d);
		});
		if (force) $('vis')._vis = null;   /* tile switch: redraw the same text */
		RvipWM.visible($('vis'), lastVis.replace(/\t(\d+)\t/gm, function (m, c) { return '\t' + PAL[+c || 7] + '\t'; }), icon);
	}   /* a key was pressed since the last autosave */

	/* ---------- input ---------- */
	function onKey(e) {
		if (!app.running || e.isComposing || e.metaKey) return;
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

	/* ---------- saves: IndexedDB (IDBFS), help (../rvip-app.js) ---------- */
	function hasSave() { try { Module.FS.stat(SAVE); return true; } catch (e) { return false; } }
	app = RvipApp({
		name: 'omega',
		save: function () { return hasSave() ? SAVE : null; },
		clear: function () { if (hasSave()) Module.FS.unlink(SAVE); },
		put: function (file, data) { Module.FS.writeFile(SAVE, data); }
	});

	/* ---------- startup ---------- */
	/* the player's name: asked once, kept in this game's IndexedDB folder (never localStorage) */
	function askName(max, bad) {
		var FS = Module.FS, f = DIR + '/web-name', n = '';
		try { n = FS.readFile(f, { encoding: 'utf8' }); } catch (e) { }
		if (!n) { n = (prompt('What is your name, adventurer?', '') || '').replace(bad, '').trim().slice(0, max); if (n) { FS.writeFile(f, n); app.sync(); } }
		return n;
	}
	window.Module = {
		om: om,
		arguments: [],
		preRun: [function () {
			var FS = Module.FS;
			Module.ENV.OMEGALIB = '/omegalib/';
			Module.ENV.HOME = DIR;
			Module.ENV.OMEGA_LINES = '40';
			Module.addRunDependency('idbfs');
			/* until 2026-09 omega kept its save in the '/save' database it shared with roguepc */
			RvipApp.mount(function (err) {
				if (err) status('Could not read saved games from IndexedDB (' + err + '). Saving may not work in this browser mode.', true);
				if (hasSave()) Module.arguments.push('omega.sav');
				var who = askName(30, /[,\n]/g);   /* Player.name = getlogin() (LOGNAME) unless rolled */
				if (who) Module.ENV.LOGNAME = who;
				try { tilesOn = Module.FS.readFile(DIR + '/web-tiles', { encoding: 'utf8' }) !== 'None'; renderTilesBtn(); } catch (e) { }
				Module.removeRunDependency('idbfs');
			}, { dir: '/save', files: ['omega.sav'] });
			FS.chdir(DIR);
		}],
		onRuntimeInitialized: function () { app.running = true; status(''); },
		print: function (s) { console.log(s); },
		printErr: function (s) { console.warn(s); },
		setStatus: function (s) { if (s && !app.running) status(s.replace(/\(\d+\/\d+\)/, '').trim() || 'Loading…'); },
		onAbort: function (what) { app.crashed(what); }
	};
	document.addEventListener('visibilitychange', function () { if (document.hidden) wantSaveFlag = true; });

	window.addEventListener('resize', function () { if (wm) wm.apply(); });
	document.addEventListener('keydown', onKey);
	document.addEventListener('DOMContentLoaded', function () {
		cv = document.createElement('canvas');
		ctx = cv.getContext('2d');
		$('btn-tiles').onclick = toggleTiles;
		renderTilesBtn();
		RvipWM.dropdown($('btn-file'), $('menu-file'));
		fetch('fonts.json').then(function (r) { return r.json(); }).then(function (list) {
			[[$('sel-font'), 'face'], [mapSel, 'mapFace']].forEach(function (a) {
				list.forEach(function (n) { var o = document.createElement('option'); o.value = n; o.textContent = n.replace(/^Web(Plus|437)_/, '').replace(/_/g, ' '); a[0].appendChild(o); });
				a[0].value = L[a[1]] || '';
			});
		}).catch(function () { });
		[[$('sel-font'), 'face'], [mapSel, 'mapFace']].forEach(function (a) {
			a[0].onchange = function () { L[a[1]] = this.value; saveLayout(); loadFace(this.value, true); this.blur(); };
		});
		$('btn-restart').onclick = function () { location.reload(); };
		document.querySelectorAll('button').forEach(function (b) {
			b.addEventListener('mousedown', function (e) { e.preventDefault(); });
		});
	});
})();
