**Remove the instructions below for your actual merge request.**

==================== INSTRUCTIONS BEGIN ========================

[GitLab action keywords that could be useful to fill this merge request](https://source.coderefinery.org/help/user/project/quick_actions)

## Title 

####  If Draft merge request
Add Draft as first word of title. 

#### If a Bugzilla ticket exists
title must be one of the following

When the MR is related but does not (alone) fully fix the bug:
```
Short description of work (BUGZ-<number>)
````

When the MR is resolves-fixes the bug:
```
Short description of work (Fixes BUGZ-<number>)
```
You can of course also mention other related bugs in the description box.

The titles and the BUGZ key-word is used in the creation of the release-notes to pick out partially and/or fully fixed bugs. 

#### If a Gitlab  issue exists
title must be one of the following (where there can be several issues)

```
Short description of work (#)
Short description of work (Fixes #)
Short description of work (Closes #)
```
Or a combination of these, example with several issues: 
```
Short description of work (#10, Fixes #1, Fixes #4).
```

## Labels
Label the work at least with the `component` label and the `type` label. If the MR is to be automatically copied to another branch, select the appropriate `copy:<branch>` label.

If you don't have labels available from the drop-down menu, you can specify the labels using syntax in bottom of the description:
/label ~foo ~"bar baz"


#### ARC component labels
Select the right ARC component labels. 

[About component labels](https://source.coderefinery.org/nordugrid/arc/wikis/contributing/overview#component-labels)

#### Type of fix labels
Select the right type labels. 

[About type labels](https://source.coderefinery.org/nordugrid/arc/wikis/contributing/overview#type-labels)

#### Copy MR labels
Select branch-labels instructing the arcbot to create merge request to additional target branch(es).

See [Rule table](https://source.coderefinery.org/nordugrid/arc/wikis/contributing/cheat-sheet#rule-table) for reference. 



## Description
* A short one-line description
* A new line
* A longer detailed description

==================== INSTRUCTIONS END ==========================



