import numpy as np
import matplotlib
import matplotlib as mpl
import matplotlib.pyplot as plt
def heatmap(data, row_labels, col_labels, ax=None,
            cbar_kw=None, cbarlabel="", **kwargs):
    """
    Create a heatmap from a numpy array and two lists of labels.

    Parameters
    ----------
    data
        A 2D numpy array of shape (M, N).
    row_labels
        A list or array of length M with the labels for the rows.
    col_labels
        A list or array of length N with the labels for the columns.
    ax
        A `matplotlib.axes.Axes` instance to which the heatmap is plotted.  If
        not provided, use current axes or create a new one.  Optional.
    cbar_kw
        A dictionary with arguments to `matplotlib.Figure.colorbar`.  Optional.
    cbarlabel
        The label for the colorbar.  Optional.
    **kwargs
        All other arguments are forwarded to `imshow`.
    """

    if ax is None:
        ax = plt.gca()

    if cbar_kw is None:
        cbar_kw = {}

    # Plot the heatmap
    im = ax.imshow(data, **kwargs)

    # Create colorbar
    cbar = ax.figure.colorbar(im, ax=ax, **cbar_kw)
    cbar.ax.set_ylabel(cbarlabel, rotation=-90, va="bottom")

    # Show all ticks and label them with the respective list entries.
    ax.set_xticks(np.arange(data.shape[1]), labels=col_labels)
    ax.set_yticks(np.arange(data.shape[0]), labels=row_labels)

    # Let the horizontal axes labeling appear on top.
    ax.tick_params(top=True, bottom=False,
                   labeltop=True, labelbottom=False)

    # Rotate the tick labels and set their alignment.
    plt.setp(ax.get_xticklabels(), rotation=-30, ha="right",
             rotation_mode="anchor")

    # Turn spines off and create white grid.
    ax.spines[:].set_visible(False)

    ax.set_xticks(np.arange(data.shape[1]+1)-.5, minor=True)
    ax.set_yticks(np.arange(data.shape[0]+1)-.5, minor=True)
    ax.grid(which="minor", color="w", linestyle='-', linewidth=3)
    ax.tick_params(which="minor", bottom=False, left=False)

    return im, cbar


def annotate_heatmap(im, data=None, valfmt="{x:.2f}",
                     textcolors=("black", "white"),
                     threshold=None, **textkw):
    """
    A function to annotate a heatmap.

    Parameters
    ----------
    im
        The AxesImage to be labeled.
    data
        Data used to annotate.  If None, the image's data is used.  Optional.
    valfmt
        The format of the annotations inside the heatmap.  This should either
        use the string format method, e.g. "$ {x:.2f}", or be a
        `matplotlib.ticker.Formatter`.  Optional.
    textcolors
        A pair of colors.  The first is used for values below a threshold,
        the second for those above.  Optional.
    threshold
        Value in data units according to which the colors from textcolors are
        applied.  If None (the default) uses the middle of the colormap as
        separation.  Optional.
    **kwargs
        All other arguments are forwarded to each call to `text` used to create
        the text labels.
    """

    if not isinstance(data, (list, np.ndarray)):
        data = im.get_array()

    # Normalize the threshold to the images color range.
    if threshold is not None:
        threshold = im.norm(threshold)
    else:
        threshold = im.norm(data.max())/2.

    # Set default alignment to center, but allow it to be
    # overwritten by textkw.
    kw = dict(horizontalalignment="center",
              verticalalignment="center")
    kw.update(textkw)

    # Get the formatter in case a string is supplied
    if isinstance(valfmt, str):
        valfmt = matplotlib.ticker.StrMethodFormatter(valfmt)

    # Loop over the data and create a `Text` for each "pixel".
    # Change the text's color depending on the data.
    texts = []
    for i in range(data.shape[0]):
        for j in range(data.shape[1]):
            kw.update(color=textcolors[int(im.norm(data[i, j]) > threshold)])
            text = im.axes.text(j, i, valfmt(data[i, j], None), **kw)
            texts.append(text)

    return texts
SDGs = ["WP","Ed","AP","GINI","YU","PS","San","AE","PL","CO2","CM","Int","Pov","Wat","Hug","Vio"]
# harvest = np.array([[0.8", 2.4, 2.5, 3.9, 0.0, 4.0, 0.0], 
#                     [2.4, 0.0, 4.0, 1.0, 2.7, 0.0, 0.0],
#                     [1.1, 2.4, 0.8, 4.3, 1.9, 4.4, 0.0],
#                     [0.6, 0.0, 0.3, 0.0, 3.1, 0.0, 0.0],
#                     [0.7, 1.7, 0.6, 2.6, 2.2, 6.2, 0.0],
#                     [1.3, 1.2, 0.0, 0.0, 0.0, 3.2, 5.1],
#                     [0.1, 2.0, 0.0, 1.4, 0.0, 1.9, 6.3]])



# harvest = np.array([[1,0.247,-0.313,-0.082,0.033,0.196,0.223,0.281,0.181,0.07,-0.247,0.238,-0.265,0.143,-0.173,-0.042],
# [0.247,1,-0.7,-0.151,0.076,0.17,0.45,0.348,0.132,0.435,-0.691,0.68,-0.554,0.608,-0.484,-0.102],
# [-0.313,-0.7,1,0.093,-0.076,-0.196,-0.542,-0.315,-0.19,-0.531,0.866,-0.773,0.669,-0.735,0.556,-0.024],
# [-0.082,-0.151,0.093,1,-0.001,-0.103,-0.282,-0.214,-0.042,-0.211,0.151,-0.194,0.286,-0.121,0.105,0.283],
# [0.033,0.076,-0.076,-0.001,1,-0.016,0.113,0.067,0.088,0.204,-0.082,0.089,-0.054,0.051,-0.05,0.032],
# [0.196,0.17,-0.196,-0.103,-0.016,1,0.253,0.218,0.33,0.174,-0.181,0.235,-0.185,0.178,-0.148,-0.115],
# [0.223,0.45,-0.542,-0.282,0.113,0.253,1,0.303,0.183,0.517,-0.534,0.601,-0.49,0.488,-0.43,-0.204],
# [0.281,0.348,-0.315,-0.214,0.067,0.218,0.303,1,0.144,0.027,-0.295,0.292,-0.304,0.224,-0.214,-0.148],
# [0.181,0.132,-0.19,-0.042,0.088,0.33,0.183,0.144,1,0.076,-0.126,0.149,-0.099,0.096,-0.12,0.053],
# [0.07,0.435,-0.531,-0.211,0.204,0.174,0.517,0.027,0.076,1,-0.544,0.671,-0.369,0.519,-0.399,-0.148],
# [-0.247,-0.691,0.866,0.151,-0.082,-0.181,-0.534,-0.295,-0.126,-0.544,1,-0.776,0.614,-0.812,0.649,0.06],
# [0.238,0.68,-0.773,-0.194,0.089,0.235,0.601,0.292,0.149,0.671,-0.776,1,-0.551,0.774,-0.587,-0.126],
# [-0.265,-0.554,0.669,0.286,-0.054,-0.185,-0.49,-0.304,-0.099,-0.369,0.614,-0.551,1,-0.484,0.315,0.119],
# [0.143,0.608,-0.735,-0.121,0.051,0.178,0.488,0.224,0.096,0.519,-0.812,0.774,-0.484,1,-0.672,-0.009],
# [-0.173,-0.484,0.556,0.105,-0.05,-0.148,-0.43,-0.214,-0.12,-0.399,0.649,-0.587,0.315,-0.672,1,0.115],
# [-0.042,-0.102,-0.024,0.283,0.032,-0.115,-0.204,-0.148,0.053,-0.148,0.06,-0.126,0.119,-0.009,0.115,1]])














harvest = np.array([[0,0,0,0.183,0.593,0.001,0,0,0.003,0.256,0,0,0,0.02,0.005,0.5],
[0,0,0,0.014,0.22,0.006,0,0,0.031,0,0,0,0,0,0,0.095],
[0,0,0,0.132,0.217,0.001,0,0,0.002,0,0,0,0,0,0,0.699],
[0.183,0.014,0.132,0,0.984,0.094,0,0,0.491,0.001,0.014,0.001,0,0.049,0.086,0],
[0.593,0.22,0.217,0.984,0,0.801,0.066,0.274,0.153,0.001,0.18,0.146,0.381,0.41,0.416,0.6],
[0.001,0.006,0.001,0.094,0.801,0,0,0,0,0.004,0.003,0,0.002,0.004,0.016,0.06],
[0,0,0,0,0.066,0,0,0,0.003,0,0,0,0,0,0,0.001],
[0,0,0,0,0.274,0,0,0,0.019,0.661,0,0,0,0,0,0.015],
[0.003,0.031,0.002,0.491,0.153,0,0.003,0.019,0,0.219,0.04,0.015,0.108,0.119,0.051,0.387],
[0.256,0,0,0.001,0.001,0.004,0,0.661,0.219,0,0,0,0,0,0,0.015],
[0,0,0,0.014,0.18,0.003,0,0,0.04,0,0,0,0,0,0,0.327],
[0,0,0,0.001,0.146,0,0,0,0.015,0,0,0,0,0,0,0.04],
[0,0,0,0,0.381,0.002,0,0,0.108,0,0,0,0,0,0,0.052],
[0.02,0,0,0.049,0.41,0.004,0,0,0.119,0,0,0,0,0,0,0.888],
[0.005,0,0,0.086,0.416,0.016,0,0,0.051,0,0,0,0,0,0,0.06],
[0.5,0.095,0.699,0,0.6,0.06,0.001,0.015,0.387,0.015,0.327,0.04,0.052,0.888,0.06,0]]);


for i in range(0,15,1):
    for j in range(0,15,1):
        if(harvest[i][j] > 0.1):
            harvest[i][j] = 0
        elif(harvest[i][j] > 0.05):
            harvest[i][j] = 1
        elif(harvest[i][j] > 0.01):
            harvest[i][j] = 2
        else:
            harvest[i][j] = 3




fig, ax = plt.subplots()
im = ax.imshow(harvest)

# Show all ticks and label them with the respective list entries
ax.set_xticks(np.arange(len(SDGs)), labels=SDGs)
ax.set_yticks(np.arange(len(SDGs)), labels=SDGs)

# Rotate the tick labels and set their alignment.
# plt.setp(ax.get_xticklabels(), rotation=45, ha="right",
#          rotation_mode="anchor")
im, cbar = heatmap(harvest, SDGs, SDGs, ax=ax,cmap="bwr")
                #    cmap="YlGn")#, cbarlabel="harvest [t/year]")


fig.tight_layout()
plt.show()