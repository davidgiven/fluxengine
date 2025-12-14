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
fig = plt.figure(figsize=(8, 8), facecolor="#aaa")

plt.imshow(
    scaledfreq,
    extent=[0, 512 / TICKS_PER_US, labels[0], labels[-1]],
    cmap="jet",
    vmin=0,
    vmax=1,
    origin="lower",
    aspect="auto",
)
plt.colorbar()
plt.ylabel("Interval period (us)")
plt.xlabel("Response (us)")
plt.grid(True, dashes=(2, 2))
plt.show()

plt.show()
