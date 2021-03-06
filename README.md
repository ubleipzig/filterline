# README

filterline filters a file by line numbers.

Taken from [here](http://unix.stackexchange.com/questions/209404/filter-file-by-line-number). There's an [awk version](https://gist.github.com/miku/bc8315b10413203b31de), too.

## Installation

There are deb and rpm [packages](https://github.com/miku/filterline/releases).

To build from source:

    $ git clone https://github.com/miku/filterline.git
    $ cd filterline
    $ make

## Usage

Note that line numbers (L) **must be sorted** and **must not contain duplicates**.

    $ filterline
    Usage: filterline FILE1 FILE2

    FILE1: line numbers, FILE2: input file

    $ cat fixtures/L
    1
    2
    5
    6

    $ cat fixtures/F
    line 1
    line 2
    line 3
    line 4
    line 5
    line 6
    line 7
    line 8
    line 9
    line 10

    $ filterline fixtures/L fixtures/F
    line 1
    line 2
    line 5
    line 6

    $ filterline <(echo 1 2 5 6) fixtures/F
    line 1
    line 2
    line 5
    line 6

## Performance

Filtering out 10 million lines from a 1 billion lines file (14G) takes about 33
seconds (dropped caches, i7-2620M):

    $ time filterline 10000000.L 1000000000.F > /dev/null
    real    0m33.434s
    user    0m25.334s
    sys     0m5.920s

A similar [awk script](https://gist.github.com/miku/bc8315b10413203b31de) takes about 2-3 times longer.

----

## Use case: data compaction

One use case for such a filter is *data compaction*. Imagine that you harvest
an API every day and you keep the JSON responses in a log.

What is a log?

> A log is perhaps the simplest possible storage abstraction. It is an
  **append-only**, totally-ordered sequence of records ordered by time.

From: [The Log: What every software engineer should know about real-time data's unifying abstraction](https://engineering.linkedin.com/distributed-systems/log-what-every-software-engineer-should-know-about-real-time-datas-unifying)

For simplicity let's think of the log as a *file*. So everytime you harvest
the API, you just *append* to a file:

```sh
$ cat harvest-2015-06-01.ldj >> log.ldj
# on the next day ...
$ cat harvest-2015-06-02.ldj >> log.ldj
...
```

The API responses can contain entries that are *new* and entries which
represent *updates*. If you want to answer the question

> What is the current state of the data?

you have to find the most recent version of each record in that log file. A
typical solution would be to switch from a file to a database of sorts and do
some kind of
[upsert](https://wiki.postgresql.org/wiki/UPSERT#.22UPSERT.22_definition).

But how about logs with 100M, 500M or billions of records? And what if you do
not want to run extra component, like a database?

You can make this process a shell one-liner, and a reasonably fast one, too.

Let's say the log entries look like this:

    $ head log.ldj
    {"id": 1, "msg": "A"}
    {"id": 2, "msg": "B"}
    {"id": 1, "msg": "C"}
    {"id": 1, "msg": "D"}
    ...

Let's say, `log.ldj` contains 1B entries (line numbers are at most ten digits) and you want to get the latest entry for each `id`. Utilizing [ldjtab](https://github.com/miku/ldjtab), the following will extract the ids along with the line number (padded), perform some munging and use `filterline` in the end to filter the original file:

    $ filterline <(ldjtab -padlength 10 -key id log.ldj | tac | \
                   sort -u -k1,1 | cut -f2 | sed 's/^0*//' | sort -n) \
                   log.ldj > latest.ldj

The filtered `latest.ldj` will contain the last entry for each `id` in the log.

## Example

We use `uldjtab` to extract the values in the same order as they appear in the
file (with `ldjtab`, we would need to use an extra `sort -k2,2` to get the same result).

```sh
$ cd fixtures
```

The `fixtures` directory contains an example JSON file `20.ldj`, with 20 lines and 6 ids from `id-0` to `id-5`.

```sh
$ cat 20.ldj
{"id": "id-0", "name": "9M9"}
{"id": "id-4", "name": "RCA"}
{"id": "id-0", "name": "2BR"}
{"id": "id-3", "name": "DQI"}
{"id": "id-0", "name": "04L"}
{"id": "id-1", "name": "ULK"}
{"id": "id-3", "name": "AO1"}
{"id": "id-4", "name": "4QY"}
{"id": "id-3", "name": "XF4"}
{"id": "id-2", "name": "SJC"}
{"id": "id-0", "name": "POL"}
{"id": "id-5", "name": "QWP"}
{"id": "id-5", "name": "ZQ0"}
{"id": "id-3", "name": "XI8"}
{"id": "id-5", "name": "CQD"}
{"id": "id-5", "name": "IQ1"}
{"id": "id-3", "name": "YXU"}
{"id": "id-2", "name": "OOX"}
{"id": "id-3", "name": "S0T"}
{"id": "id-0", "name": "QM2"}
```

First we extract the id values along with the line number. We use a padlength
of two, since we know the file has no more than 20 lines (two digit line
numbers).

```sh
$ uldjtab -key id 20.ldj
id-0    0000000001
id-4    0000000002
id-0    0000000003
id-3    0000000004
id-0    0000000005
id-1    0000000006
id-3    0000000007
id-4    0000000008
id-3    0000000009
id-2    0000000010
id-0    0000000011
id-5    0000000012
id-5    0000000013
id-3    0000000014
id-5    0000000015
id-5    0000000016
id-3    0000000017
id-2    0000000018
id-3    0000000019
id-0    0000000020
```

We then reverse the order, because in the next step we want to use `uniq`
via `sort -u` and we are interested in the latest records. The ldjtab program runs in parallel and thus cannot guarantee
order, so we need to sort the line numbers.

```sh
$ uldjtab -key id 20.ldj | tac
id-0    0000000020
id-3    0000000019
id-2    0000000018
id-3    0000000017
id-5    0000000016
id-5    0000000015
id-3    0000000014
id-5    0000000013
id-5    0000000012
id-0    0000000011
id-2    0000000010
id-3    0000000009
id-4    0000000008
id-3    0000000007
id-1    0000000006
id-0    0000000005
id-3    0000000004
id-0    0000000003
id-4    0000000002
id-0    0000000001
```

We keep only keep the latest, along with the line number.

```sh
$ ldjtab -padlength 2 -key id 20.ldj | tac | sort -u -k1,1
id-0    0000000020
id-1    0000000006
id-2    0000000018
id-3    0000000019
id-4    0000000008
id-5    0000000016
```

But actually, we are only interested in the line numbers:

```sh
$ uldjtab -key id 20.ldj | tac | sort -u -k1,1 | cut -f2
0000000020
0000000006
0000000018
0000000019
0000000008
0000000016
```

And we actually prefer the unpadded version for `filterline`, so we strip leading zeros:

```sh
$ uldjtab -key id 20.ldj | tac | sort -u -k1,1 | cut -f2 | sed 's/^0*//'
20
6
18
19
8
16
```

The `filterline` tool requires the file, that contains the line numbers to be sorted:

```sh
$ uldjtab -key id 20.ldj | tac | sort -u -k1,1 | cut -f2 | sed 's/^0*//' | sort -n
6
8
16
18
19
20
```

We know have all the ingredients we need: The original file `20.ldj` and a way
to get the line numbers of the lines we would like to keep. Using [process
substitution](https://en.wikipedia.org/wiki/Process_substitution), we can write our
one-liner:

```sh
$ filterline <(uldjtab -key id 20.ldj | tac | sort -u -k1,1 | cut -f2 | \
               sed 's/^0*//' | sort -n) 20.ldj
{"id": "id-1", "name": "ULK"}
{"id": "id-4", "name": "4QY"}
{"id": "id-5", "name": "IQ1"}
{"id": "id-2", "name": "OOX"}
{"id": "id-3", "name": "S0T"}
{"id": "id-0", "name": "QM2"}
```

Performance data points:

* The [compaction](https://gist.github.com/miku/b586c4bfe650d5456851) of a 3.4G LDJ file with 100M records with around 5M ids takes about 8 minutes.

Performance improvements (TODO):

* add pure C program as drop-in replacement for ldjtab

    This is partially addressed by [uldjtab](https://github.com/miku/ldjtab/blob/master/cmd/uldjtab/main.go), which uses
    a more performant [JSON deserializer](https://github.com/mreiferson/go-ujson).
