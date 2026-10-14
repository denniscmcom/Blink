# Designated initializers

I'm initializing almost all structures like this:

```
Foo foo = {};
foo.bar = "";
... 
```

If I know the value of each field at initialization, is better to do:

```
const Foo foo = {
	.bar = "",
};
```
