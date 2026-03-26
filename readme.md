## Lua与Qt集成框架语法规范（最终版）

本规范定义了在Lua中使用Qt类、定义可被C++调用的Qt派生类，以及编写Lua应用程序的统一语法。核心原则如下：

- **`__call` 仅用于定义新类**（`BaseClass { ... }`），**`:new` 仅用于创建实例**（`Class:new(parent)`）。
- **所有对象创建必须显式调用 `:new`**，不存在隐式实例化语法。
- **信号连接使用对象方法** `obj.connect(signal, callback)`，**信号发射使用普通函数调用** `signal(args)`。
- **类定义表只包含元信息**（`properties`、`signals`、`slots`、`methods`、`init`），UI在`init`中显式构建。
- **普通Lua方法在类定义后通过** `function ClassName:method(...)` 动态添加，不注册到Qt元对象系统。

---

### 1. 基础Qt对象使用

所有导出的Qt类（如`QWidget`、`QPushButton`、`QVBoxLayout`等）在Lua中均为全局变量，可通过`:new`创建实例。

#### 1.1 创建对象
```lua
local btn = QPushButton:new(parent)
```
- 若存在Qt注册的构造函数则原样调用
- 否则接受以父对象为唯一参数（`QObject*`）或空调用

#### 1.2 读写属性
```lua
btn.text = "New Text"
print(btn.text)
```
属性读写映射到Qt的`setProperty`/`property`。

#### 1.3 调用方法
```lua
btn:setEnabled(false)
btn:show()
```

#### 1.4 连接信号
```lua
btn.connect(btn.clicked, function(checked)
    print("Button clicked")
end)
```
- 回调函数的参数为信号参数，要求与Qt中一致。

#### 1.5 发射信号
```lua
self.counterChanged(42)
```

---

### 2. 定义新类（组件）

使用基类的`__call`操作符定义新类，传入一个包含元信息的表。

#### 2.1 基本结构
```lua
MyWidget = QWidget {
    -- 属性声明
    properties = {
        counter = { type = "int", defaultValue = 0, notify = "counterChanged" },
        text    = { type = "QString", defaultValue = "" }
    },

    -- 信号声明
    signals = {
        "counterChanged(int)",
        "maximumReached(int)"
    },

    -- 槽声明（注册到Qt元对象）
    slots = {
        function increment(self) self.counter = self.counter + 1 end,
        { name = "reset", func = function(self) self.counter = 0 end }
    },

    -- Q_INVOKABLE方法声明（注册到Qt元对象）
    methods = {
        getCounter = function(self) return self.counter end,
        setCounter = function(self, v) self.counter = v end
    },

    -- 初始化函数（实例化后自动调用）
    init = function(self)
        -- 在这里显式创建子控件、布局、连接信号
    end
}
```

#### 2.2 元信息字段说明

| 字段         | 类型     | 必选 | 说明                                                                 |
|--------------|----------|------|----------------------------------------------------------------------|
| `properties` | 表       | 否   | 定义类的属性，每个属性需指定`type`、`defaultValue`（可选）、`notify`信号名（可选） |
| `signals`    | 字符串数组 | 否   | 定义类的信号，格式`"信号名(参数类型列表)"`                           |
| `slots`      | 数组     | 否   | 定义类的槽，元素可为函数（自动命名）或`{name="...", func=...}`       |
| `methods`    | 表       | 否   | 定义Q_INVOKABLE方法，键为方法名，值为函数                           |
| `init`       | 函数     | 否   | 实例化后调用的初始化函数，用于创建子对象、连接信号、设置绑定等       |

#### 2.3 属性描述表

| 键            | 类型   | 必选 | 说明                                     |
|---------------|--------|------|------------------------------------------|
| `type`        | 字符串 | 是   | 属性类型，如"int"、"QString"、"bool"等   |
| `defaultValue`| 任意   | 否   | 默认值，若省略则使用类型的默认构造值      |
| `notify`      | 字符串 | 否   | 关联的信号名，当属性改变时发射该信号       |
| `readonly`    | 布尔   | 否   | 若为true，则属性不可写（默认false）        |

---

### 3. 添加普通Lua方法（不注册到元对象）

在类定义后，可动态添加普通方法，这些方法不会出现在Qt元对象中，仅Lua可见。
```lua
function MyWidget:normalMethod(arg)
    print("normal method", arg)
end

-- 或
MyWidget.anotherMethod = function(self)
    print("another method")
end
```

---

### 4. 构建UI

所有UI元素通过`:new`显式创建，并手动建立父子关系或布局关系：
```lua
local win = QMainWindow:new()
local central = QWidget:new(win)
win:setCentralWidget(central)

local layout = QVBoxLayout:new(central)
local btn = QPushButton:new(central, "Add")
local label = QLabel:new(central)

layout:addWidget(btn)
layout:addWidget(label)
```

---

### 5. 属性绑定

`bind`函数用于建立属性间的依赖关系，自动更新目标属性。

```lua
-- 绑定到单个属性
label.text = bind("counter")

-- 绑定到多个属性，使用转换函数
label.text = bind({"counter", "step"}, function(c, s)
    return "Count: " .. c .. " (step " .. s .. ")"
end)

-- 绑定到任意表达式（依赖自动捕获）
label.text = bind(function(self)
    return "Current: " .. self.counter
end)
```
- `bind`返回一个绑定对象，框架在实例化时解析依赖，并在依赖属性变化时自动更新目标属性。

---

### 7. 五种函数类型的语法总结

| 函数类型               | Lua调用语法                    | Lua定义位置                 | 是否注册到元对象 |
|-----------------------|--------------------------------|----------------------------|----------------|
| 普通C++成员函数        | `obj:method(args)`             | 类定义后添加                | 否             |
| Q_INVOKABLE构造函数    | `obj = Class:new(...)` | `init`字段                  | 通过new方法     |
| 信号                   | `obj.signal(args)`             | `signals`字段               | 是             |
| 槽                     | `obj:slot(args)`               | `slots`字段                 | 是             |
| Q_INVOKABLE普通函数    | `obj:method(args)`             | `methods`字段               | 是             |

---

### 8. 完整示例

```lua
local app = QApplication(arg)

-- 定义计数器面板组件
CounterPanel = QWidget {
    properties = {
        counter = { type = "int", defaultValue = 0, notify = "counterChanged" }
    },
    signals = {
        "counterChanged(int)",
        "maximumReached(int)"
    },
    slots = {
        function increment(self) self.counter = self.counter + 1 end,
        function reset(self) self.counter = 0 end
    },
    methods = {
        getCounter = function(self) return self.counter end
    },
    init = function(self)
        local layout = QVBoxLayout:new(self)
        local addBtn = QPushButton:new(self, "Add")
        local resetBtn = QPushButton:new(self, "Reset")
        local label = QLabel:new(self)

        layout:addWidget(addBtn)
        layout:addWidget(resetBtn)
        layout:addWidget(label)

        -- 属性绑定
        label.text = bind("counter", function(v)
            return "Count: " .. v
        end)

        -- 信号连接
        self.connect(addBtn.clicked, function() self:increment() end)
        self.connect(resetBtn.clicked, function() self:reset() end)

        -- 内部信号处理
        self.connect(self.counterChanged, function(value)
            if value >= 10 then
                self.maximumReached(value)   -- 发射信号
            end
        end)
    end
}

-- 普通Lua方法（不注册到元对象）
function CounterPanel:helper()
    print("Helper method called")
end

-- 主窗口
MainWindow = QMainWindow {
    init = function(self)
        local central = QWidget:new(self)
        self:setCentralWidget(central)

        local layout = QVBoxLayout:new(central)
        local panel = CounterPanel:new(central)
        local printBtn = QPushButton:new(central, "Print")

        layout:addWidget(panel)
        layout:addWidget(printBtn)

        self.connect(panel.maximumReached, function(value)
            print("Maximum reached!")
        end)

        self.connect(printBtn.clicked, function()
            print("Counter:", panel:getCounter())   -- 调用Q_INVOKABLE方法
            panel:helper()                          -- 调用普通Lua方法
        end)
    end
}

local win = MainWindow:new()
win:show()
app:exec()
```

---

### 9. 语法速查表

| 类别               | 语法示例                                                                 | 说明                                                         |
|--------------------|--------------------------------------------------------------------------|--------------------------------------------------------------|
| 创建Qt对象         | `obj = QPushButton:new(parent, "text")`                                  | 必须使用 `:new`                                               |
| 属性读写           | `obj.text = "new"`  `print(obj.text)`                                    | 映射到Qt属性系统                                              |
| 方法调用           | `obj:show()`                                                             | 冒号调用                                                     |
| 信号连接           | `obj.connect(obj.clicked, function(self, ...) ... end)`                  | 使用对象方法 `connect`                                        |
| 信号发射           | `self.counterChanged(42)`                                                | 信号作为普通函数调用                                           |
| 定义新类           | `MyClass = BaseClass { ... }`                                            | 通过基类的 `__call` 定义                                      |
| 添加普通方法       | `function MyClass:method(...) ... end`                                   | 类定义后添加，不注册到元对象                                   |
| 实例化类           | `obj = MyClass:new(parent)`                                              | 必须使用 `:new`，指定父对象                                   |
| 属性声明           | `properties = { count = { type="int", defaultValue=0 } }`                | 在类定义中声明属性                                             |
| 信号声明           | `signals = { "changed(int)" }`                                           | 在类定义中声明信号                                             |
| 槽声明             | `slots = { function inc(self) end }`                                     | 在类定义中声明槽                                               |
| Q_INVOKABLE方法声明 | `methods = { get = function(self) ... end }`                             | 在类定义中声明方法                                             |
| 初始化函数         | `init = function(self) ... end`                                          | 实例化后自动调用                                              |
| 属性绑定           | `label.text = bind("counter")`                                           | 创建绑定对象，自动更新                                        |
| C++创建组件        | `LuaComponentRegistry::createComponent("MyClass", parent)`               | 通过名称创建实例，返回 `QObject*`                            |

---

### 10. 设计原则

- **显式优于隐式**：所有对象创建、信号连接、信号发射均显式书写，无隐藏魔法。
- **职责单一**：`__call` 定义类，`:new` 创建实例，`connect` 连接信号，信号为可调用对象。
- **互操作性**：定义的类可被C++和Lua共同使用，通过Qt元对象系统无缝集成。
- **自然语法**：信号发射与普通函数调用一致，降低学习成本。

此规范为在Qt中使用Lua提供了清晰、完整、易于上手的开发体验。