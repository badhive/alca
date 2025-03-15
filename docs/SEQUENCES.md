## Sequences

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
[2025-01-01 00:00:00] [sequ] [malware.exe] name = "detect_foo_ransomware"
```

Notice that `detect_foo_rename` is marked as [private](RULES.md#private-rules). This is to avoid getting two trigger 
notifications - one for the rule and one for the sequence.

### Advantages

As mentioned in the [rule](RULES.md) documentation, rules include this feature to call another rule and use the result
as a boolean value in their own. The main caveat of this method is that only rules monitoring the same event type can
be called.

With sequences, this is not the case. All rules in a sequence can monitor separate events, which allows for
more precision when classifying behaviour unique to a particular malware sample or class of malware.

### Max Span

The integer and time identifier that you see after the colon in a sequence is known as the max span. This is the time
range that all the rules must trigger within for the sequence to match. This is particularly useful when you want
to search for particular behaviours that would be suspicious within a short time range.

The time identifier `s` denotes the number of seconds that the span range is. `m` denotes the number of minutes.

Sequences do **not** store data or event records. They simply monitor the times at which their rules were triggered,
and perform a check to see if the difference between the latest and earliest trigger times is less than the max span.

### Anonymous rules

Anonymous rules are the same as regular rules, but have no identifier. As such, they are private by default. One
anonymous rule alone being triggered will not cause a notification. Only when all rules within a sequence are triggered
will an analyst get a match.

Learn how to write rules [here](RULES.md).