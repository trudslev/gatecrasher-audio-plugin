// Gatecrasher GR-85 — knob filmstrip generator
//
// Produces the four strips shipped in assets/:
//   knob_large_160px_128f.png      160 x 20480   knurled
//   knob_small_160px_128f.png      160 x 20480   plain
//   knob_large_160px_128f-2x.png   320 x 40960   knurled
//   knob_small_160px_128f-2x.png   320 x 40960   plain
//
// Every strip is 128 frames, frame 0 = -135 deg, frame 127 = +135 deg.
// Cap-to-frame ratio is 0.75 (120 px cap in a 160 px frame) — see spec 1.3.
// The margin is not padding: it is where the cast shadow fades to zero. Do not
// re-crop the output.
//
//   npm i canvas
//   node render-knob-filmstrips.mjs [outDir]

import { createCanvas } from 'canvas';
import { writeFileSync, mkdirSync } from 'node:fs';
import { join } from 'node:path';

const FRAMES = 128;
const BASE_FRAME = 160;   // 1x frame box
const BASE_CAP = 120;     // 1x cap diameter  -> ratio 0.75
const SWEEP = 270;        // degrees, symmetric about 12 o'clock

// Per-strip pointer geometry, as fractions of cap diameter.
const STRIPS = [
  { name: 'knob_large_160px_128f', knurled: true,  pointerW: 0.048, pointerTop: 0.081, pointerLen: 0.387 },
  { name: 'knob_small_160px_128f', knurled: false, pointerW: 0.075, pointerTop: 0.125, pointerLen: 0.500 },
];

// Everything below is expressed in 1x units and multiplied by S at draw time,
// so a 2x strip is rendered at 2x — never upscaled from the 1x output.
function drawKnob(ctx, cx, cy, R, angleDeg, opts, S) {
  const { knurled, pointerW, pointerTop, pointerLen } = opts;
  const D = 2 * R;
  const pw = pointerW * D, ptop = pointerTop * D, plen = pointerLen * D;

  // 1. cast shadow — an opaque disc drawn only for its shadow, overpainted next
  ctx.save();
  ctx.shadowColor = 'rgba(0,0,0,.55)';
  ctx.shadowBlur = 11 * S;
  ctx.shadowOffsetY = 8 * S;
  ctx.beginPath();
  ctx.arc(cx, cy, R, 0, Math.PI * 2);
  ctx.fillStyle = '#0b0d0f';
  ctx.fill();
  ctx.restore();

  ctx.save();
  ctx.beginPath();
  ctx.arc(cx, cy, R, 0, Math.PI * 2);
  ctx.clip();

  // 2. cap body — radial gradient, light from upper left
  const gx = cx - R + 0.34 * D;
  const gy = cy - R + 0.22 * D;
  const g = ctx.createRadialGradient(gx, gy, 0, gx, gy, 1.18 * D);
  g.addColorStop(0, '#5b6167');
  g.addColorStop(0.46, '#2d3236');
  g.addColorStop(0.78, '#15181b');
  g.addColorStop(1, '#0b0d0f');
  ctx.fillStyle = g;
  ctx.fillRect(cx - R, cy - R, D, D);

  // 3. knurled rim — 60 serrations, fixed 6 deg step at every scale.
  //    The flute count is geometry, not resolution: 2x gives each flute more
  //    pixels, not more flutes.
  if (knurled) {
    const inner = R * 0.86;
    for (let a = 0; a < 360; a += 6) {
      const bands = [
        [0,   2.5, 'rgba(255,255,255,.07)'],
        [2.5, 3.5, 'rgba(0,0,0,.18)'],
      ];
      for (const [off, w, col] of bands) {
        const a0 = (a + off) * Math.PI / 180;
        const a1 = (a + off + w) * Math.PI / 180;
        ctx.beginPath();
        ctx.arc(cx, cy, R, a0, a1);
        ctx.arc(cx, cy, inner, a1, a0, true);
        ctx.closePath();
        ctx.fillStyle = col;
        ctx.fill();
      }
    }
  }

  // 4. top highlight / bottom occlusion
  const h = ctx.createLinearGradient(0, cy - R, 0, cy + R);
  h.addColorStop(0, 'rgba(255,255,255,.13)');
  h.addColorStop(0.35, 'rgba(255,255,255,0)');
  h.addColorStop(0.85, 'rgba(0,0,0,0)');
  h.addColorStop(1, 'rgba(0,0,0,.28)');
  ctx.fillStyle = h;
  ctx.fillRect(cx - R, cy - R, D, D);

  // 5. pointer, rotated to this frame's angle
  ctx.translate(cx, cy);
  ctx.rotate(angleDeg * Math.PI / 180);
  const pg = ctx.createLinearGradient(0, -R + ptop, 0, -R + ptop + plen);
  pg.addColorStop(0, '#ffffff');
  pg.addColorStop(1, '#c2c8cd');
  ctx.shadowColor = 'rgba(255,255,255,.30)';
  ctx.shadowBlur = 5 * S;
  ctx.fillStyle = pg;
  ctx.fillRect(-pw / 2, -R + ptop, pw, plen);
  ctx.restore();

  // 6. rim stroke, inside the clip edge so it stays crisp
  ctx.beginPath();
  ctx.arc(cx, cy, R - 1 * S, 0, Math.PI * 2);
  ctx.strokeStyle = '#08090a';
  ctx.lineWidth = 2 * S;
  ctx.stroke();
}

function renderStrip(opts, S) {
  const FRAME = BASE_FRAME * S;
  const CAP = BASE_CAP * S;
  const canvas = createCanvas(FRAME, FRAME * FRAMES);
  const ctx = canvas.getContext('2d');
  for (let i = 0; i < FRAMES; i++) {
    const angle = -SWEEP / 2 + i * SWEEP / (FRAMES - 1);
    ctx.save();
    ctx.translate(0, i * FRAME);
    drawKnob(ctx, FRAME / 2, FRAME / 2, CAP / 2, angle, opts, S);
    ctx.restore();
  }
  return canvas;
}

// Guard: the shadow must fade to zero inside the frame at every angle. This is
// the defect the 128 px strips had (border alpha 88 top / 95 bottom / 38 sides).
function maxBorderAlpha(canvas, S) {
  const FRAME = BASE_FRAME * S;
  const ctx = canvas.getContext('2d');
  let worst = 0;
  for (const fi of [0, 32, 64, 96, 127]) {
    const d = ctx.getImageData(0, fi * FRAME, FRAME, FRAME).data;
    const A = (x, y) => d[(y * FRAME + x) * 4 + 3];
    for (let i = 0; i < FRAME; i++) {
      worst = Math.max(worst, A(i, 0), A(i, FRAME - 1), A(0, i), A(FRAME - 1, i));
    }
  }
  return worst;
}

const outDir = process.argv[2] ?? '.';
mkdirSync(outDir, { recursive: true });

for (const strip of STRIPS) {
  for (const S of [1, 2]) {
    const canvas = renderStrip(strip, S);
    const file = strip.name + (S === 2 ? '-2x' : '') + '.png';
    writeFileSync(join(outDir, file), canvas.toBuffer('image/png'));
    const worst = maxBorderAlpha(canvas, S);
    console.log(
      `${file}  ${canvas.width} x ${canvas.height}  border alpha ${worst}` +
      (worst > 2 ? '  <-- SHADOW IS CLIPPED, raise the frame box' : '')
    );
  }
}
