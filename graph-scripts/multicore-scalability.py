import matplotlib.pyplot as plt
import numpy as np

plt.rcParams['font.family'] = 'DejaVu Sans'
plt.rcParams['axes.unicode_minus'] = False

# Color scheme — distinguishes architecture/mode clearly
NAVY   = '#1F618D'   # seL4_Libs SMP -virt
RED    = '#C0392B'   # seL4_Libs SMP +virt
ORANGE = '#E67E22'   # Microkit SMP +virt
TEAL   = '#16A085'   # Microkit multikernel (the winner)
ACCENT = '#C0392B'

# --- Data ------------------------------------------------------------
cores = np.array([0, 1, 2, 3])

# seL4_Libs SMP
libs_virt_on   = np.array([1094, 1760, 2579, 3405])
libs_virt_off  = np.array([ 877, 1490, 2213, 2947])

# Microkit (virtualisation = ON)
mk_smp         = np.array([1114, 1958, 2973, 3856])  # rounded from floats
mk_multikernel = np.array([ 694,  694,  695,  694])

series = [
    # (label, data, color, marker, linestyle, zorder)
    ('Microkit · multikernel (+virt)', mk_multikernel, TEAL,   'D', '-',  6),
    ('seL4_Libs · SMP (-virt)',        libs_virt_off,  NAVY,   's', '-',  5),
    ('seL4_Libs · SMP (+virt)',        libs_virt_on,   RED,    'o', '-',  5),
    ('Microkit · SMP (+virt)',         mk_smp,         ORANGE, '^', '-',  5),
]

# --- Figure ----------------------------------------------------------
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 11),
                                gridspec_kw={'height_ratios': [1.5, 1]})

# =====================================================================
# TOP PANEL — absolute cycles, all four configurations
# =====================================================================
for label, data, color, marker, ls, z in series:
    ax1.plot(cores, data, linestyle=ls, color=color, linewidth=2.3,
             marker=marker, markersize=10, markeredgecolor='white',
             markeredgewidth=1.5, label=label, zorder=z)

# Point-value labels — place above or below to avoid collisions
label_offsets = {
    'Microkit · multikernel (+virt)': (0, -20),
    'seL4_Libs · SMP (-virt)':        (0, -20),
    'seL4_Libs · SMP (+virt)':        (0,  14),
    'Microkit · SMP (+virt)':         (0,  14),
}
for label, data, color, _, _, _ in series:
    dx, dy = label_offsets[label]
    for x, y in zip(cores, data):
        ax1.annotate(str(int(y)), xy=(x, y), xytext=(dx, dy),
                     textcoords='offset points', ha='center',
                     fontsize=9.5, color=color, fontweight='bold')

# Big callout — the multikernel story (placed on the right to avoid overlapping lines)
ax1.annotate('Multikernel is essentially\nflat under interference\n(694 → 694 cycles, slope ≈ 0)',
             xy=(2.95, 694), xytext=(2.55, 1550),
             fontsize=10.5, color=TEAL, fontweight='bold', ha='center',
             bbox=dict(boxstyle='round,pad=0.5',
                       facecolor='#E8F8F5', edgecolor=TEAL, linewidth=1.2),
             arrowprops=dict(arrowstyle='->', color=TEAL, lw=1.6,
                             connectionstyle='arc3,rad=0.3'))

ax1.set_xticks(cores)
ax1.set_xlabel('Number of interfering cores', fontsize=11)
ax1.set_ylabel('Cycles (lower is better)', fontsize=11)
ax1.set_title('Multicore PPC Scalability under Core Interference — all configurations',
              fontsize=12.5, fontweight='bold')
ax1.grid(True, linestyle='--', alpha=0.4)
ax1.set_axisbelow(True)
ax1.legend(loc='upper left', fontsize=10, framealpha=0.95)
ax1.set_xlim(-0.25, 3.3)
ax1.set_ylim(0, max(mk_smp) * 1.18)

# =====================================================================
# BOTTOM PANEL — marginal cost per added interfering core
# =====================================================================
step_labels = ['0 → 1', '1 → 2', '2 → 3']
x = np.arange(len(step_labels))
n_series = len(series)
width = 0.2

for idx, (label, data, color, _, _, _) in enumerate(series):
    incs = np.diff(data)
    offset = (idx - (n_series - 1) / 2) * width
    bars = ax2.bar(x + offset, incs, width,
                   label=label, color=color, edgecolor='#333', linewidth=0.6)
    # Value labels on top
    for bar in bars:
        h = bar.get_height()
        ax2.text(bar.get_x() + bar.get_width() / 2, h + 20,
                 f'{int(h)}', ha='center', fontsize=8.5, fontweight='bold',
                 color=color)

# Horizontal reference line at zero for emphasis on multikernel
ax2.axhline(0, color='#555', linewidth=0.8)

# Highlight the multikernel "zero" row
ax2.text(-0.48, 40, 'Multikernel ≈ 0 →',
         fontsize=9.5, color=TEAL, fontweight='bold', style='italic',
         va='bottom')

ax2.set_xticks(x)
ax2.set_xticklabels([f'{lbl}\ninterfering core' for lbl in step_labels], fontsize=11)
ax2.set_ylabel('Δ cycles per added core', fontsize=11)
ax2.set_title('Marginal cost of each additional interfering core',
              fontsize=12.5, fontweight='bold')
ax2.grid(axis='y', linestyle='--', alpha=0.4)
ax2.set_axisbelow(True)
ax2.legend(loc='upper left', fontsize=9, framealpha=0.95, ncol=2)

max_inc = max(max(np.diff(s[1])) for s in series)
ax2.set_ylim(-50, max_inc * 1.35)

fig.suptitle('Architectural Impact on Multicore PPC Scalability\n'
             '(cross-address-spaces, ODROID-C4)',
             fontsize=13.5, fontweight='bold', y=0.995)

plt.tight_layout(rect=[0, 0, 1, 0.96])
out = './multicore_scalability_all.jpg'
plt.savefig(out, dpi=200, bbox_inches='tight', format='jpg', facecolor='white')
print(f'Saved: {out}')