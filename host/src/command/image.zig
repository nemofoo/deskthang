const std = @import("std");
const c = @cImport({
    @cInclude("png.h");
});

pub const ImageSize = struct {
    pub const width: usize = 240;
    pub const height: usize = 240;
    pub const pixels: usize = width * height;
    pub const bytes_per_pixel: usize = 2; // RGB565
    pub const total_bytes: usize = pixels * bytes_per_pixel;
};

const ReadContext = struct {
    file: std.fs.File,
};

pub fn decodePNG(allocator: std.mem.Allocator, file_path: []const u8) ![]u8 {
    const file = try std.fs.cwd().openFile(file_path, .{});
    defer file.close();

    var png_ptr = c.png_create_read_struct(c.PNG_LIBPNG_VER_STRING, null, null, null);
    if (png_ptr == null) return error.AllocationFailure;
    defer c.png_destroy_read_struct(&png_ptr, null, null);

    const info_ptr = c.png_create_info_struct(png_ptr);
    if (info_ptr == null) return error.AllocationFailure;

    const jmp_buf_ptr = @constCast(&c.png_jmpbuf(png_ptr)[0]);
    if (c.setjmp(jmp_buf_ptr) != 0) {
        return error.DecodingError;
    }

    var context = ReadContext{ .file = file };
    const read_func = struct {
        fn callback(png_ptr_arg: ?*c.png_struct, outBytes: [*c]u8, byte_count: usize) callconv(.C) void {
            if (png_ptr_arg) |outer_png_ptr| {
                const file_ctx = @as(*ReadContext, @ptrCast(@alignCast(c.png_get_io_ptr(outer_png_ptr))));
                _ = file_ctx.file.read(outBytes[0..byte_count]) catch {};
            }
        }
    }.callback;

    c.png_set_read_fn(png_ptr, &context, read_func);
    c.png_read_info(png_ptr, info_ptr);

    const width = c.png_get_image_width(png_ptr, info_ptr);
    const height = c.png_get_image_height(png_ptr, info_ptr);

    // Validate image dimensions
    if (width != ImageSize.width or height != ImageSize.height) {
        return error.InvalidImageDimensions;
    }

    const color_type = c.png_get_color_type(png_ptr, info_ptr);
    const bit_depth = c.png_get_bit_depth(png_ptr, info_ptr);

    // Configure transformations for RGB output
    if (bit_depth == 16)
        c.png_set_strip_16(png_ptr);
    if (color_type == c.PNG_COLOR_TYPE_PALETTE)
        c.png_set_palette_to_rgb(png_ptr);
    if (color_type == c.PNG_COLOR_TYPE_GRAY and bit_depth < 8)
        c.png_set_expand_gray_1_2_4_to_8(png_ptr);
    if (c.png_get_valid(png_ptr, info_ptr, c.PNG_INFO_tRNS) != 0)
        c.png_set_tRNS_to_alpha(png_ptr);
    if (color_type == c.PNG_COLOR_TYPE_GRAY or color_type == c.PNG_COLOR_TYPE_GRAY_ALPHA)
        c.png_set_gray_to_rgb(png_ptr);
    if (color_type == c.PNG_COLOR_TYPE_RGB_ALPHA or color_type == c.PNG_COLOR_TYPE_GRAY_ALPHA)
        c.png_set_strip_alpha(png_ptr);

    c.png_read_update_info(png_ptr, info_ptr);

    const rowbytes = c.png_get_rowbytes(png_ptr, info_ptr);
    const total_size = height * rowbytes;

    // Verify expected size
    if (total_size != ImageSize.width * ImageSize.height * 3) {
        return error.UnexpectedImageFormat;
    }

    const raw_data = try allocator.alloc(u8, total_size);
    errdefer allocator.free(raw_data);

    var row_pointers = try allocator.alloc([*c]u8, height);
    defer allocator.free(row_pointers);

    for (0..height) |i| {
        row_pointers[i] = raw_data.ptr + i * rowbytes;
    }

    c.png_read_image(png_ptr, row_pointers.ptr);

    return raw_data;
}

pub fn convertToRGB565(allocator: std.mem.Allocator, rgb888: []const u8, width: usize, height: usize) ![]u8 {
    if (rgb888.len != width * height * 3) {
        return error.InvalidInputSize;
    }

    const total_bytes = width * height * ImageSize.bytes_per_pixel;
    var rgb565_data = try allocator.alloc(u8, total_bytes);
    errdefer allocator.free(rgb565_data);

    var idx: usize = 0;
    var i: usize = 0;
    while (idx < rgb888.len) : (idx += 3) {
        const r = rgb888[idx];
        const g = rgb888[idx + 1];
        const b = rgb888[idx + 2];
        const rgb565 = rgb888_to_rgb565(r, g, b);

        rgb565_data[i] = @truncate(rgb565 & 0xFF);
        rgb565_data[i + 1] = @truncate((rgb565 >> 8) & 0xFF);
        i += 2;
    }

    return rgb565_data;
}

fn rgb888_to_rgb565(r: u8, g: u8, b: u8) u16 {
    return (@as(u16, r >> 3) << 11) | (@as(u16, g >> 2) << 5) | @as(u16, b >> 3);
}

pub const ImageError = error{
    AllocationFailure,
    DecodingError,
    InvalidImageDimensions,
    UnexpectedImageFormat,
    InvalidInputSize,
    FileNotFound,
};
