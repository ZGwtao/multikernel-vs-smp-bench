import matplotlib.pyplot as plt
import numpy as np

plt.rcParams['font.family'] = 'DejaVu Sans'
plt.rcParams['axes.unicode_minus'] = False

NAVY   = "#1E9684"    # unicore
ORANGE = "#5D22E6"   # smp (highlight — the expensive one)
ACCENT = "#F84B17"   # overhead annotation

# cross-address-spaces, virtualisation-enabled
# (label, unicore, smp)
groups = [
    ('seL4_Libs\n(seL4-14.0.0)',   699, 1094),
    ('Microkit\n(on seL4-14.0.0)', 688, 1113),
]

fig, ax = plt.subplots(figsize=(10, 6.5))

n_groups = len(groups)
width = 0.32
x = np.arange(n_groups)

unicore_vals = [g[1] for g in groups]
smp_vals     = [g[2] for g in groups]

b_uni = ax.bar(x - width/2, unicore_vals, width,
               label='Unicore', color=NAVY,   edgecolor='#333', linewidth=0.6)
b_smp = ax.bar(x + width/2, smp_vals,     width,
               label='SMP',     color=ORANGE, edgecolor='#333', linewidth=0.6)

y_max = max(unicore_vals + smp_vals)

# Value labels on top of each bar
for bars in (b_uni, b_smp):
    for bar in bars:
        h = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2, h + y_max * 0.012,
                f'{int(h)}', ha='center', fontsize=10)

# Overhead annotation: SMP vs Unicore (the main story)
for i, (uni, smp) in enumerate(zip(unicore_vals, smp_vals)):
    overhead = smp - uni
    ratio    = smp / uni
    # Curved arrow from unicore top to smp top
    ax.annotate('',
                xy=(i + width/2, smp),
                xytext=(i - width/2, uni),
                arrowprops=dict(arrowstyle='->', color=ACCENT, lw=1.8,
                                connectionstyle='arc3,rad=-0.25'))
    # Text box above
    ax.text(i, smp + y_max * 0.09,
            f'SMP ≈ {ratio:.2f}× Unicore\n(+{overhead} cycles)',
            ha='center', fontsize=11, color=ACCENT, fontweight='bold',
            bbox=dict(boxstyle='round,pad=0.4',
                      facecolor='white', edgecolor=ACCENT, linewidth=1.0))

ax.set_xticks(x)
ax.set_xticklabels([g[0] for g in groups], fontsize=12)
ax.set_ylabel('Cycles (lower is better)', fontsize=11)
ax.set_title('PPC Round-trip: Unicore vs SMP\n'
             '(cross-address-spaces, virtualisation-enabled, ODROID-C4)',
             fontsize=13, fontweight='bold')
ax.grid(axis='y', linestyle='--', alpha=0.4)
ax.set_axisbelow(True)
ax.legend(loc='upper left', fontsize=10.5, framealpha=0.95)
ax.set_ylim(0, y_max * 1.32)

plt.tight_layout()
out = './unicore_vs_smp.jpg'
plt.savefig(out, dpi=200, bbox_inches='tight', format='jpg', facecolor='white')
print(f'Saved: {out}')
