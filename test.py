
#!/usr/bin/env python3
"""
GLB to GLTF Converter Script

Converts a GLB file to GLTF format using the gltflib Python library.
"""

import argparse
import json
import os
import sys
from pathlib import Path
import gltflib


def convert_glb_to_gltf(input_path: str, output_path: str, resource_type: str = 'file') -> bool:
    """
    Convert a GLB file to GLTF format.

    Args:
        input_path: Path to the input GLB file
        output_path: Path to the output GLTF file
        resource_type: Type of resource conversion ('file', 'base64', 'external')

    Returns:
        True if conversion successful, False otherwise
    """
    try:
        # Read the GLB file
        print(f"Reading GLB file: {input_path}")

        if not os.path.exists(input_path):
            print(f"Error: Input file does not exist: {input_path}")
            return False

        # Load the GLB document
        gltf = gltflib.GLTF.load_glb(input_path)

        print("Successfully loaded GLB file")

        # Print some basic information about the loaded file
        print("Document info:")
        print(f"  - Scenes: {len(gltf.model.scenes) if gltf.model.scenes else 0}")
        print(f"  - Nodes: {len(gltf.model.nodes) if gltf.model.nodes else 0}")
        print(f"  - Meshes: {len(gltf.model.meshes) if gltf.model.meshes else 0}")
        print(f"  - Materials: {len(gltf.model.materials) if gltf.model.materials else 0}")
        print(f"  - Textures: {len(gltf.model.textures) if gltf.model.textures else 0}")
        print(f"  - Images: {len(gltf.model.images) if gltf.model.images else 0}")
        print(f"  - Buffers: {len(gltf.model.buffers) if gltf.model.buffers else 0}")
        print(f"  - Buffer Views: {len(gltf.model.bufferViews) if gltf.model.bufferViews else 0}")
        print(f"  - Accessors: {len(gltf.model.accessors) if gltf.model.accessors else 0}")

        # Create output directory if it doesn't exist
        output_path_obj = Path(output_path)
        output_dir = output_path_obj.parent

        if not output_dir.exists():
            print(f"Creating output directory: {output_dir}")
            output_dir.mkdir(parents=True, exist_ok=True)

        # Convert GLB resources to external resources before exporting
        print(f"Converting GLB resources to {resource_type} resources...")

        # Get the output filename stem for resource naming
        output_stem = output_path_obj.stem

        # Convert GLB resources based on the specified type
        if resource_type == 'file':
            # Convert to file resources (external files) - most common approach
            # The method expects a GLB resource and a base filename
            base_filename = str(output_dir / output_stem)
            glb_resource = gltf.get_glb_resource()
            gltf.convert_to_file_resource(glb_resource, base_filename)
        elif resource_type == 'base64':
            # Convert to base64 embedded resources
            glb_resource = gltf.get_glb_resource()
            gltf.convert_to_base64_resource(glb_resource)
        elif resource_type == 'external':
            # Convert to external resources
            base_filename = str(output_dir / output_stem)
            glb_resource = gltf.get_glb_resource()
            gltf.convert_to_external_resource(glb_resource, base_filename)
        else:
            print(f"Warning: Unknown resource type '{resource_type}', defaulting to 'file'")
            base_filename = str(output_dir / output_stem)
            glb_resource = gltf.get_glb_resource()
            gltf.convert_to_file_resource(glb_resource, base_filename)

        # Write the GLTF file
        print(f"Writing GLTF file: {output_path}")

        # Now export to GLTF format
        gltf.export_gltf(output_path)

        print("Successfully converted GLB to GLTF!")

        # List generated files
        output_stem = output_path_obj.stem
        print(f"\nGenerated files:")
        print(f"  - {output_path} (main GLTF file)")

        # Check for generated .bin files
        for file_path in output_dir.glob(f"{output_stem}*.bin"):
            print(f"  - {file_path} (binary data)")

        # Check for extracted images
        image_extensions = {'.png', '.jpg', '.jpeg', '.webp', '.ktx2', '.dds', '.basis'}
        for file_path in output_dir.iterdir():
            if file_path.is_file() and file_path.suffix.lower() in image_extensions:
                if file_path.name.startswith(output_stem) or file_path.name in [
                    img.name for img in (gltf.model.images or []) if hasattr(img, 'name') and img.name
                ]:
                    print(f"  - {file_path} (texture)")

        # prettify json file
        with open(output_path, 'r+') as f:
            json_data = json.load(f)

        with open(output_path, 'w') as f:
            json.dump(json_data, f, indent=4)

        return True

    except Exception as e:
        print(f"Error during conversion: {e}")
        return False


def validate_file_extensions(input_path: str, output_path: str) -> None:
    """Validate file extensions and warn if they seem incorrect."""
    input_ext = Path(input_path).suffix.lower()
    output_ext = Path(output_path).suffix.lower()

    if input_ext != '.glb':
        print(f"Warning: Input file does not have .glb extension (has {input_ext})")

    if output_ext != '.gltf':
        print(f"Warning: Output file does not have .gltf extension (has {output_ext})")


def main():
    """Main entry point for the script."""
    parser = argparse.ArgumentParser(
        description="Convert a GLB file to GLTF format.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python glb_to_gltf.py model.glb output.gltf
  python glb_to_gltf.py path/to/model.glb path/to/output.gltf
  python glb_to_gltf.py model.glb output.gltf --resource-type base64
  python glb_to_gltf.py model.glb output.gltf -r external

Resource types:
  file     - Extract resources as separate files (default)
  base64   - Embed resources as base64 in the GLTF file
  external - Create external resource references

Note: The 'file' option creates external .bin files and extracts
      textures as separate files in the same directory as the output.
        """
    )

    parser.add_argument(
        'input',
        help='Path to the input GLB file'
    )

    parser.add_argument(
        'output',
        help='Path to the output GLTF file'
    )

    parser.add_argument(
        '-r', '--resource-type',
        choices=['file', 'base64', 'external'],
        default='file',
        help='Resource conversion type: file (separate files), base64 (embedded), external (external references)'
    )

    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='Enable verbose output'
    )

    args = parser.parse_args()

    print("GLB to GLTF Converter")
    print("=" * 21)

    # Validate file extensions
    validate_file_extensions(args.input, args.output)

    # Perform the conversion
    success = convert_glb_to_gltf(args.input, args.output, args.resource_type)

    if success:
        print("\n✅ Conversion completed successfully!")
        return 0
    else:
        print("\n❌ Conversion failed!")
        return 1


if __name__ == "__main__":
    sys.exit(main())