# Linux Kernel Internals

Shared training materials for attendees of the Linux Kernel Internals course.
This repository collects the slides, practical exercises, and supplementary
notes used during the training.

## Materials

| Location | Contents |
| --- | --- |
| [exports/](exports/) | PDF slides and handouts, organized by training day |
| [practices/](practices/) | Hands-on kernel-module exercises and supporting scripts |
| [exkurs/](exkurs/) | Supplementary notes on selected Linux kernel topics |

The current material covers these themes:

- Kernel architecture, user space and kernel space, source tree navigation,
  configuration, and builds
- Kernel modules, tasks, execution contexts, preemption, synchronization, and
  race conditions
- Interrupts, deferred work, memory management, kernel allocation, and
  character devices

## Day-by-Day Release

Materials are published by training day. The material for the next day will be
made available here after the preceding day has finished. Check the relevant
directory under [exports/](exports/) and [practices/](practices/) for newly
released content.

## Using the Exercises

Individual exercises contain their own README with build and execution
instructions. Many examples are external Linux kernel modules and therefore
require matching kernel headers and appropriate permissions to load a module.

```bash
cd practices/day-2/2-03_hello
make
```

Do not load training modules on production systems. Some exercises
intentionally demonstrate unsafe behavior, such as race conditions.

## Repository Scope

This repository is intended for course participants. It is a companion to the
training sessions rather than a general-purpose Linux kernel development
framework.