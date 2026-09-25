/*
 * Omega in the browser: draws the text screen port/be_web.c sends
 * (Module.om): one canvas, the 16 PC colours of Omega's MSDOS build.
 * Keyboard, saves in IndexedDB (IDBFS, /save). Loaded before omega-core.js.
 * Structure copied from ~/Games/roguepc/web/roguepc.js.
 */
(function () {
	'use strict';

	var DIR = '/save', SAVE = DIR + '/omega.sav', ZOOM_FILE = DIR + '/web-zoom.json';
	var FONT = '"DejaVu Sans Mono", Menlo, Consolas, "Liberation Mono", monospace';
	var PAL = ['#000000', '#0000aa', '#00aa00', '#00aaaa', '#aa0000', '#aa00aa', '#aa5500', '#aaaaaa',
		'#555555', '#5555ff', '#55ff55', '#55ffff', '#ff5555', '#ff55ff', '#ffff55', '#ffffff'];
	var A_COLOR = 0x7f00, A_STANDOUT = 0x10000, A_TILE = 0x20000, MAPW = 64;
	/* map tiles: Kinder's sheet (web/tiles.js), square cells of row height,
	 * scrolled sideways to keep the player (cursor) in view, like WinOmega */
	var tilesOn = true, sheet = new Image(), tox = 0;
	try { tilesOn = localStorage.getItem('omega-tiles') !== '0'; } catch (e) { }
	sheet.onload = function () { dirty = true; draw(); };
	sheet.src = 'tiles.png';
	/* arrows and keypad = Omega's number keys (moving, and 8/2 in lists) */
	var KEYS = { ArrowUp: 56, ArrowDown: 50, ArrowLeft: 52, ArrowRight: 54, Home: 55, PageUp: 57,
		End: 49, PageDown: 51, Clear: 53, Enter: 10, Escape: 27, Backspace: 8, Delete: 8, Tab: 9 };

	var events = [], running = false, lastSave = 0;
	var cols = 80, rows = 24, scr = null, cur = { y: 0, x: 0 };
	var auto = true, cv, ctx, px = 18, cw = 11, ch = 22, dirty = true;
	var dpr = Math.max(1, Math.min(3, window.devicePixelRatio || 1));

	function $(id) { return document.getElementById(id); }
	function status(msg, isError) {
		var s = $('status');
		s.textContent = msg; s.hidden = !msg; s.classList.toggle('error', !!isError);
	}

	/* ---------- drawing ---------- */
	function measure() {
		ctx.font = px + 'px ' + FONT;
		cw = Math.ceil(ctx.measureText('M').width); ch = Math.ceil(px * 1.2);
		cv.width = cols * cw * dpr; cv.height = rows * ch * dpr;
		cv.style.width = cols * cw + 'px'; cv.style.height = rows * ch + 'px';
		ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
		ctx.font = px + 'px ' + FONT;
		ctx.textBaseline = 'top';
		dirty = true;
	}
	function fit() {
		var g = $('game'), best = 8;
		for (var p = 8; p <= 40; p++) {
			ctx.font = p + 'px ' + FONT;
			if (Math.ceil(ctx.measureText('M').width) * cols <= g.clientWidth && Math.ceil(p * 1.2) * rows <= g.clientHeight) best = p;
		}
		return best;
	}
	function draw() {
		if (!dirty || !scr) return;
		dirty = false;
		ctx.fillStyle = '#000'; ctx.fillRect(0, 0, cols * cw, rows * ch);
		for (var y = 0; y < rows; y++)
			for (var x = 0; x < cols; x++) {
				if (tilesOn && x < MAPW && scr[y * cols + x] & A_TILE) continue;
				var v = scr[y * cols + x], c = v & 0xff, fg = v >> 8 & 15, bg = v >> 12 & 7, t;
				if (!(v & A_COLOR)) fg = 7;
				if (v & A_STANDOUT) { t = fg; fg = bg; bg = t; }
				if (bg) { ctx.fillStyle = PAL[bg]; ctx.fillRect(x * cw, y * ch, cw, ch); }
				if (c > 32) { ctx.fillStyle = PAL[fg]; ctx.fillText(String.fromCharCode(c), x * cw, y * ch + (ch - px) / 2); }
			}
		if (tilesOn && drawTiles()) return;
		ctx.fillStyle = PAL[7];
		ctx.fillRect(cur.x * cw, cur.y * ch + ch - 2, cw, 2);
	}
	/* true when the cursor is on the map (drawn here as a box) */
	function drawTiles() {
		var T = ch, nx = Math.min(MAPW, Math.floor(MAPW * cw / T)), onMap = cur.x < MAPW && scr[cur.y * cols] & A_TILE;
		if (onMap) tox = Math.max(0, Math.min(MAPW - nx, cur.x - (nx >> 1)));
		ctx.imageSmoothingEnabled = false;
		ctx.textAlign = 'center';
		for (var y = 0; y < rows; y++) {
			if (!(scr[y * cols] & A_TILE)) continue;
			for (var i = 0; i < nx; i++) {
				var v = scr[y * cols + tox + i], t = OMEGA_TILES[v & 0x7fff], c = v & 0xff;
				if (t && sheet.complete && sheet.naturalWidth) ctx.drawImage(sheet, t[0] * 32, t[1] * 32, 32, 32, i * T, y * ch, T, T);
				else if (c > 32) { ctx.fillStyle = PAL[(v & A_COLOR) ? v >> 8 & 15 : 7]; ctx.fillText(String.fromCharCode(c), i * T + T / 2, y * ch + (ch - px) / 2); }
			}
		}
		ctx.textAlign = 'left';
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
		try { Module.FS.writeFile(ZOOM_FILE, JSON.stringify({ px: px })); syncFiles(); } catch (e) { }
	}

	var om = {
		init: function (c, r) {
			cols = c; rows = r; scr = new Uint32Array(c * r);
			try { px = JSON.parse(Module.FS.readFile(ZOOM_FILE, { encoding: 'utf8' })).px || 0; } catch (e) { px = 0; }
			$('game').hidden = false;
			auto = !px;
			if (auto) px = 16;
			measure();
			/* layout is ready only after this frame */
			requestAnimationFrame(function () { if (auto) { px = fit(); measure(); draw(); } });
		},
		put: function (y, x, v) { scr[y * cols + x] = v; dirty = true; },
		cursor: function (y, x) { cur.y = y; cur.x = x; dirty = true; },
		flush: function () { draw(); },
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

	window.addEventListener('resize', function () { if (auto && scr) { px = fit(); measure(); draw(); } });
	document.addEventListener('keydown', onKey);
	document.addEventListener('DOMContentLoaded', function () {
		cv = document.querySelector('#game canvas');
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
