# CSc 360: Operating Systems (Fall 2024)

# Programming Assignment 2 (P2)
# Multi-Thread Scheduling (MTS)

> **Spec Out**: Oct 4, 2024 <br>
> **Design Due**: Oct 18, 2024, 23:59 <br>
> **Code Due**: Nov 1, 2024, 23:59

## Table of Contents

+ [1. Introduction](#1-introduction)
+ [2. Schedule](#2-schedule)
+ [3. Trains](#3-trains)
    + [3.1. Reading the input file](#31-reading-the-input-file)
        + [3.1.1. Input file format](#311-input-file-format)
        + [3.1.2. An Example](#312-an-example)
    + [3.2. Simulation Rules](#32-simulation-rules)
    + [3.3. Output](#33-output)
    + [3.4. Manual Pages and other Resources](#34-manual-pages-and-other-resources)
+ [4. Submission](#4-submission)
    + [4.1. Version Control and Submission](#41-version-control-and-submission)
    + [4.2. Deliverable A (Design Due: Oct 18, 2024), 5%](#42-deliverable-a-design-due-oct-18-2024-5)
    + [4.3. Deliverable B (Code Due: Nov 1, 2024), 15%](#43-deliverable-b-code-due-nov-1-2024-15)
        + [4.3.1. Rubric](#431-rubric)
+ [5. Plagiarism](#5-plagiarism)

## 1. Introduction

In [P1 (Simple Shell Interpreter, SSI)](../p1/README.md), you have built a shell environment to interact with the host operating system. Good job! But very soon you find that `SSI` is missing one of the key features in a real multi-process operating system: **scheduling**, i.e., all processes or threads created by your `SSI` are still scheduled by the host operating system. Interested in building a multi-thread scheduling system for yourself? In P2, you will learn how to use the three programming constructs provided by the POSIX [pthread](https://man7.org/linux/man-pages/man7/pthreads.7.html) library to do so.

+ threads
+ mutexes
+ condition variables (convars)

Your goal is to construct a simulator of an automated control system for the railway track shown in Figure 1 (i.e., to emulate the scheduling of multiple threads sharing a common resource in a real OS).

![](./figures/figure1.png)

As shown in Figure 1, there are two stations on each side of the main track. The trains on the main track have two priorities: high and low.

At each station, one or more trains are loaded with commodities. Each train in the simulation commences its loading process at a common start time 0 of the simulation program. Some trains take more time to load, some less. After a train is loaded, it patiently awaits permission to cross the main track, subject to the requirements specified in Section 3.2. Most importantly, only one train can be on the main track at any given time. After a train finishes crossing, it magically disappears (i.e., the train thread finishes). You need to use `threads` to simulate the trains approaching the main track from two different directions, and your program will schedule between them to meet the requirements in Section 3.2.

You shall implement your solution in `C` programming language only. Your work will be tested on `linux.csc.uvic.ca`, which you can remotely log in by ssh. You can also access Linux computers in ECS labs in person or remotely by following the guide at https://itsupport.cs.uvic.ca/services/e-learning/login_servers/.

**Be sure to test your code on `linux.csc.uvic.ca` before submission.**

## 2. Schedule

In order to help you finish this programming assignment on time successfully, the schedule of this assignment has been synchronized with both the lectures and the tutorials.

There are four tutorials arranged during the course of this assignment, including the one on pthread.

| Date          | Tutorial                                                | Milestones by the Friday of that week                        |
|---------------|---------------------------------------------------------|--------------------------------------------------------------|
| Oct 7/8/9     | T5: P2 spec go through, `pthread`, `mutex` and `convar` | Get familiar with multi-thread programming                   |
| Oct 15/16     | T6: P2 design                                           | Get the design and code skeleton done and submit the design  |
| Oct 21/22/23  | T7: Feedback on the design and pthread programming      | Get the code almost done                                     |
| Oct 28/29/30  | T8: Testing and final instructions on the submission    | Get the final deliverable done                               |

**NOTE**: Please do attend the tutorials and follow the tutorial schedule closely.

## 3. Trains

Each train, which will be simulated by a thread, has the following attributes:

1. `Number`: An integer uniquely identifying each train, starting from `0`.
2. `Direction`:
    + If the direction of a train is `Westbound`, it starts from the East station and travels to the West station.
    + If the direction of a train is `Eastbound`, it starts from the West station and travels to the East station.
3. `Priority`: The priority of the station from which it departs.
4. `Loading Time`: The amount of time that it takes to load it (with goods) before it is ready to depart.
5. `Crossing Time`: The amount of time that the train takes to cross the main track.

Loading time and crossing time are measured in 10ths of a second. These durations will be simulated by having your threads, which represent trains, `usleep()` for the required amount of time.

### 3.1. Reading the input file

Your program (`mts`) will accept only one command line parameter:

+ The parameter is the name of the input file containing the definitions of the trains.

#### 3.1.1. Input file format

The input files have a simple format. Each line contains the information about a single train, such that:

+ The first field specifies the direction of the train. It is one of the following four characters: `e`, `E`, `w`, or `W`.
    + `e` or `E` specifies a train headed East (East-Bound): `e` represents an east-bound low-priority train, and `E` represents an east-bound high-priority train;
    + `w` or `W` specifies a train headed West (West-Bound): `w` represents a west-bound low-priority train, and `W` represents a west-bound high-priority train.
+ Immediately following (separated by a space) is an integer that indicates the loading time of the train.
+ Immediately following (separated by a space) is an integer that indicates the crossing time of the train.
+ A newline (\n) ends the line.

Trains are numbered sequentially from 0 according to their order in the input file.

You can use `strtok()` to tokenize each line input. More efficiently, you can use `fscanf()`.

#### 3.1.2. An Example

The following file specifies three trains, two headed East and one headed West.

```
e 10 6
W 6 7
E 3 10
```

It implies the following list of trains:

![](./figures/figure2.png)

**Note**: Observe that `Train 2` is actually the first to finish the loading process.

### 3.2. Simulation Rules

The rules enforced by the automated control system are:
+ Only one train is on the main track at any given time.
+ Only loaded trains can cross the main track.
+ If there are multiple loaded trains, the one with the high priority crosses.
+ If two loaded trains have the same priority, then:
    + If they are both traveling in the same direction, the train which finished loading first gets the clearance to cross first. If they finished loading at the same time, the one that appeared first in the input file gets the clearance to cross first.
    + If they are traveling in opposite directions, pick the train which will travel in the direction opposite of which the last train to cross the main track traveled. If no trains have crossed the main track yet, the Westbound train has the priority.
+ To avoid starvation, if there are two trains in the same direction traveling through the main track **back to back**, the trains waiting in the opposite direction get a chance to dispatch one train if any.

### 3.3. Output

For the example, shown in Section 3.1.2, the correct output is:

```
00:00:00.3 Train  2 is ready to go East
00:00:00.3 Train  2 is ON the main track going East
00:00:00.6 Train  1 is ready to go West
00:00:01.0 Train  0 is ready to go East
00:00:01.3 Train  2 is OFF the main track after going East
00:00:01.3 Train  1 is ON the main track going West
00:00:02.0 Train  1 is OFF the main track after going West
00:00:02.0 Train  0 is ON the main track going East
00:00:02.6 Train  0 is OFF the main track after going East
```

You must:

+ print the arrival of each train at its departure point (after loading) using the format string, prefixed by the simulation time:
    ```
    "Train %2d is ready to go %4s"
    ```
+ print the crossing of each train using the format string, prefixed by the simulation time:
    ```
    "Train %2d is ON the main track going %4s"
    ```
+ print the arrival of each train (at its destination) using the format string, prefixed by the simulation time:
    ```
    "Train %2d is OFF the main track after going %4s"
    ```

where

+ There are only two possible values for direction: "East" and "West"
+ Trains have integer identifying numbers. The ID number of a train is specified implicitly in the input file. The train specified in the first line of the input file has ID number 0.
+ Trains have loading and crossing times in the range of `[1, 99]`.

### 3.4. Manual Pages and other Resources

Be sure to study the man pages for the various functions to be used in the assignment. For example, the man page for `pthread_create` can be found by typing the command:

```
$ man pthread_create
```

At the end of this assignment you should be familiar with the following functions:

1. File access functions:

    + (a) `atoi`
    + (b) `fopen`
    + (c) `feof`
    + (d) `fgets` and `strtok` and more efficiently you can use `fscanf`
    + (e) `fwrite` or `fprintf`
    + (f) `fclose`

2. Thread creation functions:

    + (a) `pthread_create`
    + (b) `pthread_exit`
    + (c) `pthread_join`

3. Mutex manipulation functions:

    + (a) `pthread_mutex_init`
    + (b) `pthread_mutex_lock`
    + (c) `pthread_mutex_unlock`

4. Condition variable manipulation functions:

    + (a) `pthread_cond_init`
    + (b) `pthread_cond_wait`
    + (c) `pthread_cond_broadcast`
    + (d) `pthread_cond_signal`

It is absolutely critical that you read the man pages, and attend the tutorials. Your best source of information, as always, is the man pages.

For help with the POSIX interface (in general): http://www.opengroup.org/onlinepubs/007908799/

For help with POSIX threads: http://www.opengroup.org/onlinepubs/007908799/xsh/pthread.h.html

A good overview of pthread can be found at: http://computing.llnl.gov/tutorials/pthreads/

## 4. Submission

### 4.1. Version Control and Submission

Same as `P1` and `W1`, you shall use the department GitLab server `https://gitlab.csc.uvic.ca/` for version control during this assignment.

You shall create a new repository named `p2` under your personal assignment group, e.g., `https://gitlab.csc.uvic.ca/courses/2024091/CSC360_COSI/assignments/<netlink_id>/p2`, and commit both your design document (Deliverable A) and code (Deliverable B) to this repository.

You are required to commit and push your work-in-progress code at least weekly to your p2 GitLab repository.

The submission of your `p2` assignment is the last commit before the due date. If you commit and push your code after the due date, the last commit before the due date will be considered as your final submission.

The TA will clone your `p2` repository, test and grade your code on `linux.csc.uvic.ca`.

**Note**: Use [`.gitignore`](https://git-scm.com/docs/gitignore/en) to ignore the files that should not be committed to the repository, e.g., the executable file, `.o` object files, etc.

### 4.2. Deliverable A (Design Due: Oct 18, 2024), 5%

You will write a design document which answers the following questions. It is recommended that you think through the following questions very carefully before answering them.

Unlike P1, no amount of debugging will help after the basic design has been coded. Therefore, it is very important to ensure that the basic design is correct. Answering the following questions haphazardly will basically ensure that Deliverable B does not work.

So think about the following for a few days and then write down the answers.

+ How many threads are you going to use? Specify the work that you intend each thread to perform.
+ Do the threads work independently? Or, is there an overall “controller” thread?
+ How many mutexes are you going to use? Specify the operation that each mutex will guard.
+ Will the main thread be idle? If not, what will it be doing?
+ How are you going to represent stations (which are collections of loaded trains ready to depart)? That is, what type of data structure will you use?
+ How are you going to ensure that data structures in your program will not be modified concurrently?
+ How many convars are you going to use? For each convar:
    + Describe the condition that the convar will represent.
    + Which mutex is associated with the convar? Why?
    + What operation should be performed once `pthread_cond_wait()` has been unblocked and re-acquired the mutex?
+ In 15 lines or less, briefly sketch the overall algorithm you will use. You may use sentences such as:
    ```
    If train is loaded, get station mutex, put into queue, release station mutex.
    ```
    The marker will not read beyond 15 lines.

The design counts for `5%`.

Please write your answers of the above questions in plain text format in a single file named **`p2-A-<netlink_id>-V#.txt`** and commit it to your `p2` repository root folder.

### 4.3. Deliverable B (Code Due: Nov 1, 2024), 15%

Your submission will be marked by an automated script.

+ Your submission **must** include a `README.md` file that describes whether you have implemented `mts` correctly. Including a `README.md` file is mandatory, and you will lose marks if you do not submit it in your GitLab repository.

+ Your submission **must** compile successfully with a single `make` command. The `Makefile` should be in the root directory of your `p2` repository. The produced executable file **must** be named as `mts` and be placed in the root directory of your `p2` repository. Otherwise, you will lose marks.

+ Your submission **must** include a test input file named `input.txt` in the root directory of your `p2` repository. The format of your test input file must follow the same format as defined in Section 3.1.1. You should include an estimated execution time of your `mts` with your test input file in the `README.txt` file.

+ Your `mts` **must** save the output results in a file named `output.txt` after the simulation is finished. Your `mts` can still output info on `stdout`, but only the content in `output.txt` will be evaluated by the automated script.

Note:

The automated marker will give a time quota of 1 minute for your program to run on a given input. This time quota is given so that non-terminating programs can be killed. Since your program simulates train crossing delays in 10ths of a second, this should not be an issue, at all.

The markers will read your C code after the script has run to ensure that the pthread library is used as required.

The code counts for `15%`.

### 4.3.1. Rubric

#### Basic Functions (Total: 5 Mark, 1 Mark each)

+ Your submission contains a `README.md` (or `README.txt`) file that describes whether you have implemented `mts` correctly.
+ Your submission can be successfully compiled by `make` without warnings or errors. The produced executable is named as `mts`.
+ Your `mts` can be executed with one command line parameter, i.e., the path to the input file. E.g., `./mts input.txt`.
+ Your `mts` saves the simulation output to `output.txt` following the format defined in Section 3.3. If `mts` is invoked multiple times on different input files, you should overwrite `output.txt` with the results from the latest run.
+ Your `mts` has no deadlocks, and the simulation can complete within 1 minute using an input file that satisfies this condition.

#### Simulation Results (Total: 10 Mark)

+ Your `mts` will be tested against 5 different test cases, each worth 2 marks, with different numbers of trains and different loading and crossing times.

## 5. Plagiarism

This assignment is to be done individually. You are encouraged to discuss the design of your solution with your classmates on Teams, but each person must implement their own assignment.

The markers will submit the code to an automated plagiarism detection program. Do not request/give source code from/to others; Do not put/use code at/from public repositories such as GitHub or so.

The End
