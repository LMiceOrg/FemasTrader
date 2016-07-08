#App
控制终端

#netmd
行情信号接收程序

		%%mermaid
		graph LR
		m[Model]-->|Register: netmd-symbol |b(netmd)


#FM2Trader
飞马2交易程序

		%%mermaid
		graph TD
		Trader[App] -.->|1 OnConnected|Spi(Model)
		Spi -->|1.1 login|Trader
		
		Trader1[App] -.->|2 OnRspUserLogin|Spi1(Model)
		Spi1 -->|2.1 Go to work|Trader1	

