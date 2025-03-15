## ALCA

ALCA is a modular rule engine built for dynamic malware analysis. Inspired by YARA, ALCA features a rich set of
inspection capabilities that can be applied to any user-defined event. In other words, ALCA can be used for general
event-based monitoring, as long as it can receive and interpret events from an external source (which we
refer to as a [sensor](#sensors)).

### Install

Alca binaries for Windows and Linux (64-bit / 32-bit) are provided in the 
[Releases](https://github.com/badhive/alca/releases/latest) page. To build the project yourself, you will need
Git, a C compiler (gcc, MinGW, MSVC) and CMake installed. 

```sh
# clone the repository
git clone https://github.com/badhive/alca
cd alca

# fetch missing dependencies
git submodule update --init

# build
cmake -B build . 
cmake --build build/ --target alca alcac
```

The executable will be located in the `./build` directory.

### Rules and Sequences

Writing rules and sequences is simple and intuitive. Have a look at the examples in the following sections.

#### Rules

An ALCA rule will typically include an **event type** that is being monitored, and a **condition** which makes use of
the received event's properties. Here's an example:

```
event file

rule detect_foo_ransomware : file {
    file.action == file.FILE_RENAME and file.new_name iendswith ".foo"
} 
```

Let's break it down:
1. The rule first checks if the file action matches the user-defined enum `FILE_RENAME` (meaning, a file was renamed).
If this condition is true, then the second condition (on the right hand side of `and`) is evaluated.
2. The rule then does a **case-insensitive** check on the new file name to see if it ends with ".foo" (int other words, if
the extension has changed). If both these conditions are true, then the rule is flagged as true and we receive a
notification similar to the one below:

```
[2025-01-01 00:00:00] [rule] name = "detect_foo_ransomware"
```

#### Sequences

In ALCA, a sequence is a set of rules that must be satisfied in the order that they appear, within an optional time frame. 
A sequence is evaluated to `true` when this happens. This behaviour is similar to Elastic EQL's 
[sequence](https://www.elastic.co/guide/en/elasticsearch/reference/current/eql-syntax.html#eql-sequences).

Sequences are considered essential to minimising false positives and improving the granularity of your detections.
They can contain either predefined rules or anonymous rules, which are defined inline. Here's an example:

```
event file

private rule detect_foo_rename : file {
    file.action == file.FILE_RENAME and file.new_name iendswith ".foo"
}

sequence detect_foo_ransomware : 5s [
    detect_foo_rename,
    :file { file.action == file.FILE_CREATE and file.name iequals "foo_ransom_note.txt" }
]
```

This sequence:
1. Monitors the **private** rule `detect_foo_rename`. This checks if a file was renamed with the extension ".foo".
2. Monitors a rule that checks if a file named "foo_ransom_note.txt" was created. If both of these are triggered 
sequentially and within 5 seconds of each other, then we should see exactly **one** notification.

```
[2025-01-01 00:00:00] [sequ] name = "detect_foo_ransomware"
```

Notice that `detect_foo_rename` is marked as `private`. This is to avoid getting two trigger notifications - one for
the rule and one for the sequence.

### Sensors

Sensors are applications that act as a server to receive binary samples, run them and stream events to the ALCA engine.
ALCA CLI by default uses TCP sockets for local and remote communication, however if you wish to use the ALCA library,
you'll be able to fully customise the way in which you send files and receive events.

ALCA requires a sensor to execute files and thus receive events. A sensor must be installed and run separately
so that ALCA can connect to it.

If connected in local mode (-l), ALCA submits the path of the file requested to be analysed to the sensor. 
The sensor (assumed to be installed on the same machine) can then run this file and report any events it gathers
back to the engine via the same socket. If connected in remote mode (-r), ALCA sends the entire requested file over
the socket to be received and run by the sensor.

ALCA's own flagship sensor is currently in active development.

### Credits / References / Inspirations

- https://github.com/PCRE2Project/pcre2 Regex library
- https://github.com/VirusTotal/yara YARA, static analysis engine
- https://www.elastic.co/guide/en/elasticsearch/reference/current/eql.html Elastic EQL