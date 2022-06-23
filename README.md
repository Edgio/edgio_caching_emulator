# Edgio Caching Emulator
A web-caching policy emulator.

[![standard-readme
compliant](https://img.shields.io/badge/readme%20style-standard-brightgreen.svg?style=flat-square)](https://github.com/RichardLitt/standard-readme)

The Edgio Caching Emulator (ECE) is a tool for comparing the impacts of
arbitrary caching policies on cache performance for fixed workloads. In
particular, it is intended to consume log data from production caching servers
and provide insight into how the cache would behave, both in terms of
traditional cache metrics (hit rate, eviction rate, etc.) and more tangential
metrics (disk writes, origin pulls, etc.).

## Table of Contents

- [Background](#background)
- [Install](#install)
- [Usage](#usage)
- [Contribute](#contribute)
- [License](#license)

## Background

This project was developed to allow for the easy implementation and comparison
of arbitrary caching policies. Doing this in an isolated environment allows one to
avoid having to implement each potential policy in production, which can be both
time consuming and risky. Furthermore, the system is log-driven, and is designed to
be driven by production traffic traces. Since it uses these production
workloads, we dubbed this system an _emulator_, rather than a simulator.

The Edgio Caching Emulator includes the following features.
* **Cache Admission**
 * **N-hit caching**: items are cached on the n-th request
 * **Probabilistic caching**: items are cached based on a pre-defined probability value
* **Cache Eviction**
 * **FIFO**: Items are purged in first-in-first-out order
 * **Regular LRU**: items are purged in a least recently used/requested fashion
 * **Size-based LRU**: during each purge, the largest sized item from a list of X least recently used items is deleted

The emulator provides data from various metrics to show the performance of such
techniques over time, including hit and byte-hit ratios (overall and per customer), cache reads and writes, and reads from origin

## Install

To install the ECE, first clone the repo:

```git clone https://github.com/edgioinc/edgio_caching_emulator.git ```

Simply hop into the directory and run make:

``` cd edgio_caching_emulator ```

``` make ```

The Makefile will use clang++ by default, but you can override the default with another compiler of your choice:

``` CPP=g++ make ```

## Usage

The ECE comes with a ready made set of cache admission (probabilistic,
second-hit caching) and eviction (LRU, FIFO, S4LRU) policies. To run a ready
made example script will perform an example using Second-Hit Caching and LRU:

``` ./run_em.sh <log directory> 2hc_lru ```

Where `<log directory>` is a directory containing logs of the appropriate
format.  Sample logs are provided in the directory `input_request_sequence`.

Output will be placed in `out/<timestamp>`.

A script is included that provides a simple plot of the hit rates, along with
examples of how to parse the output format. Please note, this script requires matplotlib and numpy - they are not required for the emulator itself. Run this script with the following command:

`python analysis/parse.py out/<timestamp>/`

### Data

* Input to the emulator is a list of individual asset request entries. Each entry is a new line, with the following columns:

1. Timestamp
2. Size
3. Destination port (e.g. 80 or 443) - unused column
4. Cache status code (e.g.: TCP_HIT/200, TCP_HIT/206 TCP_MISS/200, TCP_PARTIAL_HIT/200, etc.)
5. Bytes out (bytes served from the requested file, could be smaller in case of range requests)
6. URL (asset requested)

* Output is stored in a .dat file (e.g. `out/29_Nov_17-18_54/lru_2hc.dat`).

Below is an example of the dat file entry:
`Emulator_periodic_reporting 1503625504  32720333373821  213587  |      ghr 0.623736  0.998646   0.720994  0.999307  |  cache 0.623736 0.998646 133222 80365 32676017633906 44315739915 63820417982 42267494 0 44315739915  : 2hc_r    ot 0.252592  : lru 229997792359 0.0311111`

In the above sample output log entry, each line is an aggregate of 15 minutes of requests processed.
Each line begins with `emulator_periodic_reporting` key word.
This line can then be parsed by any plotting script to generate figures. In this repo one such script is `analysis/parse.py`.

Below is the definition of each column in the output line above.

1.  emulator_periodic_reporting - static keyword
2.  1503625504 - timestamp of the output (once very 15 mins)
3.  32720333373821 - Total traffic seen in the last timestamp (in bytes)
4.  213587 - Total numer of URLs/files seen in last chunk
5.  | - delimiter (below we have global stats)
6.  ghr - global stats (hit rate and byte-hit rate)
7.  0.623736 - total hit rate
8.  0.998646 - total byte hit rate
9.  0.720994 - total hit rate - perfect cache (admission on first hit, no eviction)
10. 0.999307 - total byte hit rate - perfect cache
11. | delimiter (below we have cache specific stats; followed by two colons (:))
12. cache - has cache specific stats (next 10 columns, i.e. 13 through 22.)  Each new cache/storage tier we add begins with keyword 'cache'. The output here is ordered how they were tied together in the emulator. Legend for the next 10 columns can be found here: `analysis/parse.py#L79`
23. : - delimiter (below have the cache admission specific stats)
24. 2hc_rot - cache admission scheme name
25. 0.252592 - bloom filter (used for keeping track of hit count) fill rate
26. : - delimiter (below we have the cache eviction specific stats)
27. lru - cache eviction scheme name
28. 229997792359 - size of all content on disk
29. 0.0311111 - oldest file age

* Sample input logs can be found in directory: `input_request_sequence`
* Sample emulator output can be found in directory: `out`


### Adding Additional Experiments


Additional experiments can be written and placed in the src/ directory. The idea
is similar to NS-3, where individual experiments can be written as stand alone
main() functions that instantiate an emulator object, add caches, and then
consume stdin.

The `run_em.sh` script can then be called with a list of binaries, i.e.

`./run_sum.sh <log directory> lru_2hc my_experimet1 my_experiment2`

### Adding Policies

Additional policies can be added to `lib/` (and headers to
`include/`, as needed) and can inherit from the appropriate class in
`cache_policy.cc` (i.e. admission or eviction). `cache_policy.cc` defines two
classes that provide the necessary framework for creating a cache policy. In particular, mimicking the definitions of the following components would be a good place to start: 
* size admission (`include/size_admission.h` and `lib/size_admission.cc`) for admission policies.
* fifo (`include/fifo.h` and `lib/fifo.cc`) for eviction policies.

New policies that implement custom `periodic_output` functions will also need
new parsing functions; these can be implemented at the top of parse.py.

## Contribute

Please refer to [the contributing.md file](Contributing.md) for information
about how to get involved. We welcome issues, questions, and pull requests. Pull
Requests are welcome. We are particularly interested in implementations of
additional caching policies.

## License

This project is licensed under the terms of the Apache 2.0 open source license.
Please refer to [LICENSE](LICENSE) for the full terms.

