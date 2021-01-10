import numpy
import matplotlib.pyplot as plt
import matplotlib.animation as animation

TICK_FREQUENCY = 12e6
TICKS_PER_US = TICK_FREQUENCY / 1e6
print(TICKS_PER_US)

# Load data.
data = numpy.loadtxt(open("driveresponse.csv", "rb"), delimiter=",", skiprows=1)

labels = data[:, 0]
frequencies = data[:, 1:]

# Scale the frequencies.
def scaled(row):
    m = row.mean()
    if m != 0:
        return row / m
    else:
        return row

scaledfreq = numpy.array([scaled(row) for row in frequencies])

# Create new Figure with black background
fig = plt.figure(figsize=(8, 8), facecolor='#aaa')

plt.imshow(scaledfreq, extent=[0, 512/TICKS_PER_US, labels[0], labels[-1]], cmap='jet',
           vmin=0, vmax=1, origin='lower', aspect='auto')
plt.colorbar()
plt.ylabel("Interval period (us)")
plt.xlabel("Response (us)")
plt.show()

## Add a subplot with no frame
#ax = plt.subplot(111, frameon=False)
#
## Generate random data
#X = numpy.linspace(-1, 1, frequencies.shape[-1])
#G = 1.5 * numpy.exp(-4 * X ** 2)
#
#
## Generate line plots
#lines = []
#for i in range(len(frequencies)):
#    # Small reduction of the X extents to get a cheap perspective effect
#    xscale = 1 - i / 300.
#
#    # Same for linewidth (thicker strokes on bottom)
#    lw = 1.5 - i / 200.0
#
#    # Scale.
#    row = frequencies[i]
#    max = numpy.amax(row)
#    if max == 0:
#        Y = 1
#    else:
#        Y = 5 / max
#    line, = ax.plot(xscale*X, i + Y*frequencies[i], color="w", lw=lw)
#    lines.append(line)
#
## Set y limit (or first line is cropped because of thickness)
##ax.set_ylim(-1, 100)
#
## No ticks
##ax.set_xticks([])
##ax.set_yticks([])
#
## 2 part titles to get different font weights
##ax.text(0.5, 1.0, "MATPLOTLIB ", transform=ax.transAxes,
##ha="right", va="bottom", color="w",
##family="sans-serif", fontweight="light", fontsize=16)
##ax.text(0.5, 1.0, "UNCHAINED", transform=ax.transAxes,
##ha="left", va="bottom", color="w",
##family="sans-serif", fontweight="bold", fontsize=16)

plt.show()
