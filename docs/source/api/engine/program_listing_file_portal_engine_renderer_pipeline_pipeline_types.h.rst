
.. _program_listing_file_portal_engine_renderer_pipeline_pipeline_types.h:

Program Listing for File pipeline_types.h
=========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_engine_renderer_pipeline_pipeline_types.h>` (``portal\engine\renderer\pipeline\pipeline_types.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   
   #include "portal/core/strings/string_id.h"
   
   namespace portal::renderer
   {
   class ShaderVariant;
   }
   
   namespace portal::renderer
   {
   enum class PrimitiveTopology
   {
       None,
       Points,
       Lines,
       Triangles,
       LineStrip,
       TriangleStrip,
       TriangleFan
   };
   
   enum class DepthCompareOperator
   {
       None,
       Never,
       NotEqual,
       Less,
       LessOrEqual,
       Greater,
       GreaterOrEqual,
       Equal,
       Always,
   };
   
   
   enum class PipelineStage
   {
       None,
       TopOfPipe,
       DrawIndirect,
       VertexInput,
       VertexShader,
       TessellationControlShader,
       TessellationEvaluationShader,
       GeometryShader,
       FragmentShader,
       EarlyFragmentTests,
       LateFragmentTests,
       ColorAttachmentOutput,
       ComputeShader,
       Transfer,
       BottomOfPipe,
       Host,
       AllGraphics,
       AllCommands,
       AccelerationStructureBuild,
       RayTracingShader,
       MeshShader
   };
   
   enum class ResourceAccessFlags
   {
       None,
       IndirectCommandRead,
       IndexRead,
       VertexAttributeRead,
       UniformRead,
       InputAttachmentRead,
       ShaderRead,
       ShaderWrite,
       ColorAttachmentRead,
       ColorAttachmentWrite,
       DepthStencilAttachmentRead,
       DepthStencilAttachmentWrite,
       TransferRead,
       TransferWrite,
       HostRead,
       HostWrite,
       MemoryRead,
       MemoryWrite,
       AccelerationStructureRead,
       AccelerationStructureWrite
   };
   }
