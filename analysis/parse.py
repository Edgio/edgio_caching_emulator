""" Parser for emulator output """
# Copyright 2021 Edgecast Inc
# Licensed under the terms of the Apache 2.0 open source license
# See LICENSE file for terms.

import glob
import os.path
import sys

import plot

#########################

# Parsers for specific policies

def parse_lru(segment):
    """ Parser for LRU periodic output"""
    lru = {}

    data = segment.split()

    lru["size"] = int(data[1])
    lru["oldest_file_age"] = float(data[2])

    return lru

def parse_2hc(segment):
    """ Parser for 2hc periodic output"""
    second_hit = {}

    data = segment.split()
    second_hit["fill_perc"] = float(data[1])

    return second_hit

# A dictionary that holds all the different parsing functions
POLICY_FUNC = {
    "lru": parse_lru,
    "2hc": parse_2hc,
    "2hc_rot": parse_2hc, #NOTE: uses same func
    }
###########################

def detect_versions(directory):
    """ Given a directory, detect what runs are in it """
    wild_dir = os.path.join(directory, "*.dat")
    out_paths = glob.glob(wild_dir)
    # Strip off the .dat and take that as the label
    out_names = [os.path.basename(x)[:-4] for x in out_paths]

    # zip them together
    return zip(out_paths, out_names)

def proc_cache_chunk(key, chunk, index):
    """ Given a cache chunk, figure out what it is, get the stuff """
    data_dict = {}

    # General simulation stuff
    if key == "emulator_periodic_reporting":
        data = chunk.split()[1:]
        data_dict["time"] = int(data[0])
        data_dict["traffic"] = int(data[1]) # NOTE: Does not currently update!
        data_dict["urls"] = int(data[2])
        return (key, data_dict)

    # Global cache
    elif key == "ghr":
        data = chunk.split()[1:]
        data_dict["total_hit_ratio"] = float(data[0])
        data_dict["total_byte_hit_ratio"] = float(data[1])
        data_dict["infinite_hit_ratio"] = float(data[2])
        data_dict["infinite_byte_hit_ratio"] = float(data[3])
        # Key is ghr
        return (key, data_dict)

    # Each individual cache
    elif key == "cache":
        segments = chunk.split(":")
        # First, let's parse out the generic features
        data = segments[0].split()[1:]

        # Generic cache
        data_dict["hit_ratio"] = float(data[0])
        data_dict["byte_hit_ratio"] = float(data[1])
        data_dict["hits"] = int(data[2])
        data_dict["misses"] = int(data[3])
        data_dict["byte_hits"] = int(data[4])
        data_dict["byte_misses"] = int(data[5])
        # Storage info
        # All these are in #512 byte ops
        data_dict["num_reads"] = int(data[6])
        data_dict["num_writes"] = int(data[7])
        data_dict["num_purges"] = int(data[8])
        # This is in bytes
        data_dict["origin_reads"] = int(data[9])

        # Ok now read from your admission set
        admit_name = segments[1].split()[0]
        data_dict[admit_name] = POLICY_FUNC[admit_name](segments[1])

        # Next the eviction set
        evict_name = segments[2].split()[0]
        data_dict[evict_name] = POLICY_FUNC[evict_name](segments[2])

    # here the key out will be the index less one, since generic is 0
    return (index - 1, data_dict)

def parse_emulator_log(log_file):
    """ Spin through a sim log and get out the goods"""

    time_series_data = []

    # Open it up and spin through
    with open(log_file) as log_f:
        for line in log_f:
            clean_line = line.strip()
            # Let's get all the periodic outputs
            if clean_line.startswith("emulator_periodic_reporting"):
                # Ok, first, let's split on pipes as they represent each cache
                line_dict = {}
                cache_chunks = clean_line.split("|")
                # Loop through each of the chunks
                for index, cache in enumerate(cache_chunks):
                    key = cache.split()[0]
                    key, data = proc_cache_chunk(key, cache, index)
                    line_dict[key] = data
                time_series_data.append(line_dict)

    # Normalize the times
    start = min([x["emulator_periodic_reporting"]["time"] for x in time_series_data])
    for line in time_series_data:
        line_item = line["emulator_periodic_reporting"]
        line_item["time"] = (line_item["time"] - start)/60


    return time_series_data

def plot_hit_ratio(log_list, label_list):
    """ Plot the global system hit ratio """

    final_list = []

    time_x = None

    for log_item, label in zip(log_list, label_list):
        log_data = log_item

        time_x = [x["emulator_periodic_reporting"]["time"] for x in log_data]
        time_x = [float(x)/1440 for x in time_x]

        write = [y["ghr"]["total_hit_ratio"] for y in log_data]

        matched_labels = [
            (write, label.title()),
            ]

        final_list.append(matched_labels)

    plot.plot_time_cmp(time_x,
                       final_list,
                       xlabel="Time (Days)",
                       ylabel="Overall hit-ratio",
                       #ncol=len(final_list),
                       ncol=2,
                       loc="upper right",
                       #line="536870912000",
                      )
def main():
    """ Main func """
    # Lookup everything
    version_files = detect_versions(sys.argv[1])

    # Big lists to hold the logs and the labels
    log_list = []
    label_list = []

    #  Spin through the data files and parse each one, put the results in the
    # appropriate list
    for out_file, label in version_files:
        # Parse the log, save the output
        log_out = parse_emulator_log(out_file)
        log_list.append(log_out)
        label_list.append(label)

    # Call a plotting function on our list of data and labels
    plot_hit_ratio(log_list, label_list)

if __name__ == "__main__":
    main()
