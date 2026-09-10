"""Package existing UE viewport frames with Pillow. Does not generate VFX images."""
from pathlib import Path
from PIL import Image, ImageOps, ImageDraw

root = Path(__file__).resolve().parents[2]
source = root / 'Saved/RadiantDissolve/Preview'
output = root / 'Docs/VFX'
output.mkdir(parents=True, exist_ok=True)
frames = [Image.open(source / f'frame_{i:03d}.png').convert('RGB') for i in range(72)]
frames[0].save(output / 'RadiantDissolve_Preview.gif', save_all=True,
               append_images=frames[1:], duration=[40, 40, 40, 40, 40, 50] * 12,
               loop=0, optimize=False, disposal=2)

sheet = Image.new('RGB', (960, 580), '#10151d')
draw = ImageDraw.Draw(sheet)
for index, (frame, label) in enumerate([(8, '0.00 s / Reset'), (18, '0.42 s / Heat'),
                                        (28, '0.83 s / Dissolve front'), (56, '2.00 s / Gone')]):
    x, y = (index % 2) * 480, (index // 2) * 290
    sheet.paste(frames[frame].resize((480, 270), Image.Resampling.LANCZOS), (x, y))
    draw.text((x + 12, y + 273), label, fill='#edf1ff')
sheet.save(output / 'RadiantDissolve_Stages.jpg', quality=95)
print(output / 'RadiantDissolve_Preview.gif')
