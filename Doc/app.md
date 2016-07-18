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
		graph LR
		App(App) -->|初始化注册订阅|init_trader
		init_trader-->	|下单|order_insert
		init_trader --> 	|全平|flatten_all
		init_trader --> 	|交易品种|trade_instrument
		Trader-.->|OnFrontDisconnected|Spi0(App)
		Spi0-->|Quit|End(结束)
		Trader(交易系统) -.->|1 OnFrontConnected|Spi(App)
		Spi -->|1.1 login|login
		
		Trader -.->|2 OnRspUserLogin|Spi1(App)
		Spi1 -->|2.1 Start working|work(开始工作)

		Strategy(策略) -->|3 Publish trading instrument|Spi2(App)

		Spi3(App) -->|4 Publish OnRsp, OnRt系列|LMice[Daemon]

##回调函数
注意使用如下三种类型转换模式

dynamic_cast
static_cast
reinterpret_cast

## Response & Return处理

直接写入日志

#备注

飞马系	统结构体为基准