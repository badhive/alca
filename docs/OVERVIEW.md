## Overview

This gives a brief overview of the terminology used when referring to parts of this project.

| Terminology     | Description                                                                                                                                                  |
|-----------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Event           | Arbritary data received from a sensor that contains information about the execution state of a program                                                       |
| Event type      | An string idenfifier for the type of data contained in an event received from the sensor                                                                     |
| Module          | An user-defined extension to the ALCA framework, responsible for deserialising an event's data into native ALCA objects to be interpreted by the rule engine |
| Rule            | A named boolean expression that is executed against event data                                                                                               |
| Match / Trigger | When a rule evaluates to `true`                                                                                                                              |
| Sequence        | A list of rules, where the order of the rules determines whether the sequence triggers or not                                                                |
| Sensor          | A program or service responsible for executing a file and reporting actions and state changes back to ALCA                                                   |
