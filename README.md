## ALCA

ALCA is a modular rule engine built for dynamic malware analysis. Inspired by YARA, ALCA features a rich set of
inspection capabilities that can be applied to any user-defined event. In other words, ALCA can be used for general
event-based monitoring, as long as it can receive and interpret events from an external source (which we
refer to as a [sensor](#sensors)).

### Install

ALCA has been tested on Windows and Linux. 
To build the project yourself, you will need Git, a C compiler (gcc, MinGW, etc.) and CMake installed. 

```sh
# clone the repository
git clone https://github.com/badhive/alca
cd alca

# fetch missing dependencies
git submodule update --init

# build
cmake -S . -B build/ -DCMAKE_BUILD_TYPE=Release
cmake --build build/ --target alca
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
    file.action == file.FileRename and file.new_name iendswith ".foo"
} 
```

Let's break it down:
1. The rule first checks if the file action matches the user-defined enum `FileRename` (meaning, a file was renamed).
If this condition is true, then the second condition (on the right hand side of `and`) is evaluated.
2. The rule then does a **case-insensitive** check on the new file name to see if it ends with ".foo" (in other words, if
the extension has changed). If both these conditions are true, then the rule is flagged as true, and we receive a
notification similar to the one below:

```
[2025-01-01 00:00:00] [rule] [malware.exe] name = "detect_foo_ransomware"
```

The available fields for each event (\*Event) and their types can be found in the [ALCA FlatBuffers schema file](cli/alca.fbs).
The following functions are also supported:

| Module    | Args        | Description                                                 |
|-----------|-------------|-------------------------------------------------------------|
| `network` | `nslookup4` | Performs a DNS lookup for a provided IPv4 address and port. |
| `network` | `nslookup6` | Performs a DNS lookup for a provided IPv6 address and port. |

Here's an example of a more complex rule that makes use of all modules that ALCA currently supports:

```
event file
event process
event network
event registry

private rule test_rule : file {
	file.action == file.FileCreate and file.path icontains "MAL.TXT"
}

sequence test_sequence [
    test_rule
    :file { file.action == file.FileDelete and file.name iequals "MAL2.TXT" }
    :network {
        network.action == network.NetConnect and
        not network.ipv6 and
        network.nslookup4( network.remote_addr, network.remote_port ) icontains "googleusercontent.com"
    }
    :registry {
        registry.action == registry.RegOpenKey and
        registry.key_path iendswith "Software\\Microsoft\\Windows\\CurrentVersion\\Run"
    }
    :registry { registry.action == registry.RegSetValue and registry.value_name == "TestBinary" }
]
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
    file.action == file.FileRename and file.new_name iendswith ".foo"
}

sequence detect_foo_ransomware : 5s [
    detect_foo_rename,
    :file { file.action == file.FileCreate and file.name iequals "foo_ransom_note.txt" }
]
```

This sequence:
1. Monitors the **private** rule `detect_foo_rename`. This checks if a file was renamed with the extension ".foo".
2. Monitors a rule that checks if a file named "foo_ransom_note.txt" was created. If both of these are triggered 
sequentially and within 5 seconds of each other, then we should see exactly **one** notification.

```
[2025-01-01 00:00:00] [sequ] [malware.exe] name = "detect_foo_ransomware"
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

ALCA's own flagship sensor, Governor, is available [here](https://github.com/badhive/governor).

### Documentation

View the full documentation [here](docs).

### Issues

If you find any problems with the project, feel free to open an issue on the 
[issues page](https://github.com/badhive/alca/issues).

### Credits / References / Inspirations

- https://github.com/PCRE2Project/pcre2 Regex library
- https://github.com/VirusTotal/yara YARA, static analysis engine
- https://www.elastic.co/guide/en/elasticsearch/reference/current/eql.html Elastic EQL