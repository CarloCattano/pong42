import numpy as np
import sys
from PIL import Image

def reorganize_font_atlas(input_image_path, output_image_path, charset_offset=0, scale_font=1.0):

    img = Image.open(input_image_path)
    img_width, img_height = img.size


    desired_cell_size = 128  # Example, character cell size in pixels


    num_cols = img_width // desired_cell_size
    num_rows = img_height // desired_cell_size

    # Atlas size (number of characters)
    atlas_size = (num_cols, num_rows)


    dynamic_cell_width = img_width // num_cols
    dynamic_cell_height = img_height // num_rows


    print(f"Image size: {img_width}x{img_height}")
    print(f"Calculated Atlas Size (cols x rows): {atlas_size}")
    print(f"Calculated Dynamic Cell Size: {dynamic_cell_width}x{dynamic_cell_height}")

    grayscale_img = img.convert('L')
    grayscale_pixels = np.array(grayscale_img)

    # Create a list of characters based on luminance and assign them to new positions
    luminance_map = []
    for row in range(num_rows):
        for col in range(num_cols):

            x = col * dynamic_cell_width
            y = row * dynamic_cell_height


            char_block = grayscale_pixels[y:y + dynamic_cell_height, x:x + dynamic_cell_width]


            avg_luminance = np.mean(char_block)
            luminance_map.append((avg_luminance, (row, col)))

    luminance_map.sort()
    reorganized_chars = []
    for i, (_, (row, col)) in enumerate(luminance_map):
        reorganized_chars.append((row, col, i))  # Add the new index and position

    new_atlas = Image.new('RGB', (img_width, img_height))

    for new_index, (old_row, old_col, new_pos) in enumerate(reorganized_chars):
        x = old_col * dynamic_cell_width
        y = old_row * dynamic_cell_height

        char_image = img.crop((x, y, x + dynamic_cell_width, y + dynamic_cell_height))

        new_row = new_pos // num_cols
        new_col = new_pos % num_cols

        new_atlas.paste(char_image, (new_col * dynamic_cell_width, new_row * dynamic_cell_height))

    new_atlas.save(output_image_path)
    print(f"Reorganized font atlas saved to {output_image_path}")


arguments = sys.argv[1:]

input_image_path = arguments[0]
output_image_path = arguments[1]
reorganize_font_atlas(input_image_path, output_image_path)

