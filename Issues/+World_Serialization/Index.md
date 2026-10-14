# World serialization

Once I've implemented prefab serialization, I could implement world serialization.

I'll need a way to mark prefabs at runtime when they are loaded into the world,
so when I serialize the world I just can use the prefab hash instead of
serializing it again inside world.
