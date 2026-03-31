local obj = qt.MyDynamicObject:new()
print(qt.MyDynamicObject.super)
qt.QObject.test = 1
print(qt.MyDynamicObject.test)
print(qt.MyDynamicObject.test2)
obj.value = 123
print('obj value', obj.value)

obj.onValueChanged = function(newValue)
    print('Signal received: value changed to', newValue)
end
obj:connect(obj.valueChanged, obj.onValueChanged)
print("1234")
obj:valueChanged(456.0)
obj:doSomething()
print('Signal test completed')
print('Lua Qt bridge test completed successfully')