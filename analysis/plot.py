""" Plotting tools for cache emulator """
# Copyright 2021 Edgecast Inc
# Licensed under the terms of the Apache 2.0 open source license
# See LICENSE file for terms.

from matplotlib import pyplot as plt
import numpy as np

def plot_cdf(data, xlim=None, ylim=(0, 1.1), xlabel=None, ylabel=None,
             filename=None, log=False):
    """ Basic single CDF plotting funtion """
    # Do the necessary data wiggling for a CDF
    sorted_x = np.sort(data)
    yvals = np.arange(len(sorted_x))/float(len(sorted_x))


    fig = plt.figure(figsize=(6, 2.5))

    # Set the plot params as needed
    if xlim != None:
        plt.xlim(xlim)
    if ylim != None:
        plt.ylim(ylim)
    if xlabel != None:
        plt.xlabel(xlabel)
    if ylabel != None:
        plt.ylabel(ylabel)

    # If log
    if log:
        plt.xscale("log")


    # Plot it

    plt.plot(sorted_x, yvals, lw=2, color='k')
    if filename != None:
        plt.savefig(filename, bbox_inches="tight")
    plt.show()

def plot_multi_cdf(data, xlim=None, ylim=(0, 1.1), xlabel=None, ylabel=None,
                   filename=None, log=False, loc=None, legend=True,
                   file_size_labels=False, ncol=1):
    """ Basic single CDF plotting funtion """

    # Standalone
    fig = plt.figure(figsize=(6, 2.5))
    # For 3 across a page
    #fig = plt.figure(figsize=(4,2.5))
    # For 4 Acriss
    #fig = plt.figure(figsize=(3.7,3.7))

    # Set the plot params as needed
    if xlim != None:
        plt.xlim(xlim)
    if ylim != None:
        plt.ylim(ylim)
    if xlabel != None:
        plt.xlabel(xlabel)
    if ylabel != None:
        plt.ylabel(ylabel)
    # If log
    if log:
        plt.xscale("log", basex=2)


    # File size labels
    if file_size_labels:
        labels = ["1B", "1KB", "1MB", "1GB"]
        label_loc = [1, 1024, 1024*1024, 1024*1024*1024]

        plt.xticks(label_loc, labels)


    style_list = ["-", "--", ":", "-.", None]
    dash_list = [[12, 6, 3, 6, 3, 6],]

    for index, condition_set in enumerate(data):

        style = style_list[index % len(style_list)]
        if index > len(style_list) - 1:
            color = "gray"
        else:
            color = 'k'

        for y_data, label in condition_set:
            # Do the necessary data wiggling for a CDF
            sorted_x = np.sort(y_data)
            yvals = np.arange(len(sorted_x))/float(len(sorted_x))

            if style != None:
                plt.plot(sorted_x, yvals, lw=2, label=label, ls=style,
                         color=color)
            else:
                plt.plot(sorted_x, yvals, lw=2, label=label,
                         dashes=dash_list[0], color=color)

            #max_list.append(sorted_x[-1]) # Its sorted, so biggest at end

    if loc is None:
        loc = "lower right"

    if legend:
        plt.legend(loc=loc, frameon=False, ncol=ncol)

    if filename != None:
        plt.savefig(filename, bbox_inches="tight")
    plt.show()

def plot_time(datax, data_y_list, xlim=None, ylim=(0, 1.1), xlabel=None, ylabel=None,
              filename=None, label_list=None):
    """ Basic single CDF plotting funtion """
    # Check the labels
    if len(data_y_list) > 1 and label_list is None:
        raise RuntimeError("Must give labels with more than one y list!")


    # Do the necessary data wiggling for a CDF
    fig = plt.figure(figsize=(6, 2.5))

    # Set the plot params as needed
    if xlim != None:
        plt.xlim(xlim)
    if ylim != None:
        plt.ylim(ylim)
    if xlabel != None:
        plt.xlabel(xlabel)
    if ylabel != None:
        plt.ylabel(ylabel)

    # Plot it
    if len(data_y_list) == 1:
        plt.plot(datax, data_y_list[0], lw=2, color='k')
    else:
        # Spin through the list
        for index, y_data in enumerate(data_y_list):
            plt.plot(datax, y_data, lw=2, label=label_list[index])
        # Turn the legend on
        plt.legend(loc="lower right")


    if filename is not None:
        plt.savefig(filename, bbox_inches="tight")

    plt.show()

def plot_time_cmp(datax, data_y_list,
                  xlim=None, ylim=None,
                  xlabel=None, ylabel=None, filename=None,
                  ncol=3, loc="lower right", xlog=False,
                  ylog=False,
                  file_size_labels=False,
                  y_file_size=False,
                  line=None):
    """ Basic single CDF plotting funtion """
    # Check the labels
    #if len(data_y_list) > 1 and label_list is None:
    #    raise RuntimeError("Must give labels with more than one y list!")

    # Lets find the min, in case lists are different sizes
    big_list = []
    for c in data_y_list:
        len_list = [len(y_data) for y_data, label in c]
        big_list.extend(len_list)
    min_val = min(big_list)


    # Do the necessary data wiggling for a CDF
    fig = plt.figure(figsize=(6, 2.5))
    #fig = plt.figure(figsize=(4, 2.5))


    # Set the plot params as needed
    if xlim != None:
        plt.xlim(xlim)
    if ylim != None:
        plt.ylim(ylim)
    if xlabel != None:
        plt.xlabel(xlabel)
    if ylabel != None:
        plt.ylabel(ylabel)
    if xlog:
        plt.xscale("log", basex=2)
    if ylog:
        plt.yscale("log", basey=2)


    # File size labels
    if file_size_labels:
        labels = ["32GB", "256GB", "1TB", "8TB"]
        label_loc = [32, 256, 1028, 8192]
        #labels = ["1GB", "4GB", "16GB", "64GB"]
        #label_loc = [1, 4, 16, 64]

        plt.xticks(label_loc, labels)

    if y_file_size == True:
        labels = ["32GB", "256GB", "1TB", "4TB"]
        label_loc = [32, 256, 1028, 4096]

        labels = ["4TB", "32TB", "256TB", "2PB"]
        label_loc = [2**42, 2**45, 2**48, 2**51]
        plt.yticks(label_loc, labels)
        pass

    style_list = ["-", "--", ":", "-.", None]
    dash_list = [[12, 6, 3, 6, 3, 6],]

    if line != None:
        plt.axhline(y=line, lw=1, ls=":", color="gray")

    for index, condition_set in enumerate(data_y_list):
        style = style_list[index % len(style_list)]
        if index > len(style_list) - 1:
            color = "gray"
        else:
            color = 'k'
        for y_data, label in condition_set:
            #plt.plot(datax[:min_val], y_data[:min_val], lw=2, label=label,
            #         ls=style,
            #         #marker='o', ms=4, markeredgewidth=0.0,
            #         color=color,
            #         )

            if style != None:
                plt.plot(datax[:min_val], y_data[:min_val], lw=2, label=label, ls=style,
                         color=color)
            else:
                plt.plot(datax[:min_val], y_data[:min_val], lw=2, label=label,
                         dashes=dash_list[0], color=color)
    # Spin through the list
    #for index, y_data in enumerate((zip(data_y_list1, data_y_list2))):
    #    #print y_data[0]
    #    #print y_data[1]
    #    #print "*********************"
    #    plt.plot(datax, y_data[0], lw=2, label=label_list1[index])
    #    plt.plot(datax, y_data[1], lw=2, ls="--", label=label_list2[index])


    # Turn the legend on
    plt.legend(loc=loc, ncol=ncol, frameon=False, numpoints=1)


    if filename is not None:
        plt.savefig(filename, bbox_inches="tight")

    plt.show()

def plot_raw(data):


    plt.plot(data, lw=2, color='k')

    #plt.savefig(filename, bbox_inches="tight")
    plt.show()

def plot_xy(x_data, y_data,
            xlim=None, ylim=None,
            xlabel=None, ylabel=None, filename=None,
            xlog=False,
            ncol=3, loc="lower right",
            file_size_labels=False):

    # Do the necessary data wiggling for a CDF
    fig = plt.figure(figsize=(6, 2.5))

    # Set the plot params as needed
    if xlim != None:
        plt.xlim(xlim)
    if ylim != None:
        plt.ylim(ylim)
    if xlabel != None:
        plt.xlabel(xlabel)
    if ylabel != None:
        plt.ylabel(ylabel)
    if xlog != False:
        plt.xscale('log', nonposx='clip', basex=2)
    # File size labels
    if file_size_labels == True:
        labels = ["32GB", "256GB", "1TB", "4TB"]
        label_loc = [32, 256, 1028, 4096]

        plt.xticks(label_loc, labels)

    plt.axvline(x=2**24, lw=1, ls=":", color='k')

    for index, hit_rate_list in enumerate(y_data):
        color = str(float(len(y_data) - index)/float(len(y_data)))
        print color

        plt.plot(x_data, hit_rate_list, lw=2, color=color)


    # Turn the legend on
    #plt.legend(loc=loc, ncol=ncol, frameon=False)


    if filename is not None:
        plt.savefig(filename, bbox_inches="tight")

    plt.show()

def plot_scatter(x_data, y_data, xlim=None, ylim=None, xlabel=None, ylabel=None,
                 filename=None, log=False, loc=None,
                 file_size_labels=False):
    """ Scatter plot of the given data """

    fig = plt.figure(figsize=(6, 2.5))

    # Down Sample:
    x_data = x_data[::1000]
    y_data = y_data[::1000]



    # Set the plot params as needed
    if xlim != None:
        plt.xlim(xlim)
    if ylim != None:
        plt.ylim(ylim)
    if xlabel != None:
        plt.xlabel(xlabel)
    if ylabel != None:
        plt.ylabel(ylabel)
    # If log
    if log == True:
        plt.xscale("log")
        plt.yscale("log")


    # File size labels
    if file_size_labels == True:
        labels = ["1B", "1KB", "1MB", "1GB"]
        label_loc = [1, 1024, 1024*1024, 1024*1024*1024]

        plt.xticks(label_loc, labels)
        plt.yticks(label_loc, labels)

    plt.plot(x_data, y_data, ls='None', marker='o', color='k', mfc='gray')


    if loc == None:
        loc = "lower right"

    #plt.legend(loc=loc, frameon=False, ncol=1)

    if filename != None:
        plt.savefig(filename, bbox_inches="tight")
    plt.show()

