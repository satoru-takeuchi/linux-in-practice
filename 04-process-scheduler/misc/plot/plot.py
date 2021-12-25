#!/usr/bin/python3

import numpy as np
from PIL import Image
import matplotlib
import os
import sys

matplotlib.use('Agg')

import matplotlib.pyplot as plt
from matplotlib import rcParams

plt.rcParams['font.family'] = "sans-serif"
plt.rcParams['font.sans-serif'] = "TakaoPGothic"

def plot_sched(concurrency):
    fig = plt.figure()
    ax = fig.add_subplot(1,1,1)
    for i in range(concurrency):
        x, y = np.loadtxt("{}.data".format(i), unpack=True)
        ax.scatter(x,y,s=1)
    ax.set_title("タイムスライスの可視化(並列度={})".format(concurrency))
    ax.set_xlabel("経過時間[ミリ秒]")
    ax.set_xlim(0)
    ax.set_ylabel("進捗[%]")
    ax.set_ylim([0,100])
    legend = []
    for i in range(concurrency):
        legend.append("負荷処理"+str(i))
    ax.legend(legend)

    # Ubuntu 20.04のmatplotlibのバグを回避するために一旦pngで保存してからjpgに変換している
    # https://bugs.launchpad.net/ubuntu/+source/matplotlib/+bug/1897283?comments=all
    pngfilename = "sched-{}.png".format(concurrency)
    jpgfilename = "sched-{}.jpg".format(concurrency)
    fig.savefig(pngfilename)
    img = Image.open(pngfilename).convert("RGB").save(jpgfilename)
    os.remove(pngfilename)
