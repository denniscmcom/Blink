# Avoid function overloading

There are some free functions for basic containers that share the same name like
`push`. Instead we should prefix them with the name of the container, for
example `dyn_array_push`.
