cbuffer gConstants : register(b0)
{
	uint flash_invert;
	uint transparent;
};

StructuredBuffer<float4> gPalette : register(t0);
StructuredBuffer<uint> gDisplay : register(t1);
StructuredBuffer<uint> gMask : register(t2);

struct VS_OUTPUT
{
	float4 pos : SV_Position;
	float2 tex : TEXCOORD0;
	uint width : TILE_W;
	uint height : TILE_H;
	uint offset_x : OFFSET_X;
	uint offset_y : OFFSET_Y;
	uint offset : DATA_OFFSET;
};

float4 main (VS_OUTPUT input) : SV_Target
{
	uint x = input.offset_x + input.tex.x * input.width;
	uint y = input.offset_y + input.tex.y * input.height;

	// DATA: 00 00 00 Y7  Y6 Y2 Y1 Y0   Y5 Y4 Y3 X7  X6 X5 X4 X3
	//                --  -- -- -- --   Y7 Y6 -- --  -- -- -- --   << 5
	//                --  -- -- -- --   -- -- Y5 Y4  Y3 -- -- --   << 2
	//                --  -- -- -- --   -- -- -- --  -- Y2 Y1 Y0   << 8
	//                --  -- -- -- --   X7 X6 X5 X4  X3 -- -- --   >> 3
	uint data_offset =
		input.offset +
		((y & 0xc0) << 5) +
		((y & 0x38) << 2) +
		((y & 0x07) << 8) +
		((x & 0xf8) >> 3);
	uint data_dword = gDisplay[data_offset >> 2];
	uint data_mask = 1 <<
		((x & 0x18) |
		(7 - (x & 0x07)));

   // ATTR: 00 00 00 00  00 00 Y7 Y6   Y5 Y4 Y3 X7  X6 X5 X4 X3
   //                          -- --   Y7 Y6 Y5 Y4  Y3 -- -- --   << 2
   //                          -- --   X7 X6 X5 X4  X3 -- -- --   >> 3
	uint attr_offset =
		input.offset + 0x1800 +
		((y & 0xf8) << 2) +
		((x & 0xf8) >> 3);
	uint attr_dword = gDisplay[attr_offset >> 2];
	uint attr_byte = (attr_dword >> (x & 0x18)) & 0xff;

	uint col_shift = (data_dword & data_mask) ? 0 : 3;
	if (attr_byte & 0x80 && flash_invert)
		col_shift ^= 3;
	uint col_idx = (attr_byte >> col_shift) & 7;

	if (col_idx == 0 && transparent && (gMask[data_offset >> 2] & data_mask))
		discard;	// masked black is transparent

	if (attr_byte & 0x40)		// bright?
		col_idx += 8;			// if so, use second half of colours

	return gPalette[col_idx];
}
