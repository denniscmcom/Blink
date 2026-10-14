# Vertex for terrain

Currently, I am using `Vertex_UV` for generating the terrain, so most of fields
are unused. It's not that important but it's a quick fix.

## To reproduce

- Launch the engine
- The following warnings will be logged:

```    
vkCreateGraphicsPipelines(): pCreateInfos[0].pVertexInputState Vertex attribute at location 4 not consumed by vertex shader. 
```  

## To fix

Replace `Vertex_UV` with `Vertex_Spatial` in the terrain pipeline.
