#memo

## [app] work flow

DEDS --> mono

[netmd]

[strategy]

[trader]

		%%mermaid
		graph LR
		femas-->|MD|netmd
		netmd -->|event1|strategy
		strategy-->|event2|trader
		trader-->|Insert|femas


merge event time

		%%mermaid
		graph LR
		femas -->|MD|app(netmd--strategy--trader)
		app-->|Insert|femas


trader:logined



## memory perf

minimize footprint


## network perf 

High level protocol --> OpenOnLoad

Cons femas protocol ?

## host perf 

HT

perf --> critical path --> analysis

hugepage --> page fault

