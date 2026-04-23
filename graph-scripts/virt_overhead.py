import matplotlib.pyplot as plt
import numpy as np

plt.rcParams['font.family'] = 'DejaVu Sans'
plt.rcParams['axes.unicode_minus'] = False

NAVY   = '#1F618D'   # +virt
LIGHT  = '#AED6F1'   # -virt
ACCENT = '#C0392B'   # overhead annotation

# seL4-13.0.0 data
# (config, virt, novirt)
# unicore_pairs = [
#     ('cross-AS', 647, 632),
#     ('same-AS',  617, 601),
# ]
# smp_pairs = [
#     ('cross-AS', 1102, 887),
#     ('same-AS',   954, 878),
# ]
# seL4-14.0.0 data
# (config, virt, novirt)
unicore_pairs = [
    ('cross-AS', 699, 676),
    ('same-AS',  666, 649),
]
smp_pairs = [
    ('cross-AS', 1094, 877),
    ('same-AS',   947, 860),
]

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(13, 6.2), sharey=True)

# Compute a single y-limit so both panels use the same scale
GLOBAL_YMAX = max([p[1] for p in unicore_pairs + smp_pairs] +
                  [p[2] for p in unicore_pairs + smp_pairs])

def draw_panel(ax, pairs, title):
    x = np.arange(len(pairs))
    width = 0.36
    virt_vals   = [p[1] for p in pairs]
    novirt_vals = [p[2] for p in pairs]
    labels      = [p[0] for p in pairs]

    b1 = ax.bar(x - width/2, virt_vals,   width,
                label='+virt (virtualisation enabled)',
                color=NAVY,  edgecolor='#333', linewidth=0.6)
    b2 = ax.bar(x + width/2, novirt_vals, width,
                label='-virt (no virtualisation)',
                color=LIGHT, edgecolor='#333', linewidth=0.6)

    y_max = GLOBAL_YMAX

    # Value labels on top of each bar
    for bars in (b1, b2):
        for bar in bars:
            h = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2, h + y_max * 0.012,
                    f'{int(h)}', ha='center', fontsize=10)

    # Overhead annotation between each pair
    for i, (v, nv) in enumerate(zip(virt_vals, novirt_vals)):
        overhead = v - nv
        pct = overhead / nv * 100
        # Draw a double-arrow showing the delta
        ax.annotate('',
                    xy=(i - width/2, v),
                    xytext=(i - width/2, nv),
                    arrowprops=dict(arrowstyle='<->', color=ACCENT, lw=1.6))
        # Overhead text box above the +virt bar
        ax.text(i, v + y_max * 0.08,
                f'Δ = +{overhead} cycles\n(+{pct:.1f}%)',
                ha='center', fontsize=10.5, color=ACCENT, fontweight='bold',
                bbox=dict(boxstyle='round,pad=0.35',
                          facecolor='white', edgecolor=ACCENT, linewidth=0.9))

    ax.set_xticks(x)
    ax.set_xticklabels(labels, fontsize=12)
    ax.set_ylabel('Cycles (lower is better)', fontsize=11)
    ax.set_title(title, fontsize=13, fontweight='bold')
    ax.grid(axis='y', linestyle='--', alpha=0.4)
    ax.set_axisbelow(True)
    ax.legend(loc='lower right', fontsize=9.5, framealpha=0.95)
    ax.set_ylim(0, y_max * 1.28)

draw_panel(ax1, unicore_pairs, 'Unicore PPC Round-trip')
draw_panel(ax2, smp_pairs,     'SMP PPC Round-trip')

# fig.suptitle('Virtualisation Overhead on seL4-13.0.0 (ODROID-C4, no interference)',
#              fontsize=14, fontweight='bold', y=0.995)
fig.suptitle('Virtualisation Overhead on seL4-14.0.0 (ODROID-C4, no interference)',
             fontsize=14, fontweight='bold', y=0.995)

plt.tight_layout(rect=[0, 0, 1, 0.95])
# out = './sel4_13_virt_overhead.jpg'
out = './sel4_14_virt_overhead.jpg'
plt.savefig(out, dpi=200, bbox_inches='tight', format='jpg', facecolor='white')
print(f'Saved: {out}')