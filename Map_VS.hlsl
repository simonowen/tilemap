#define ZX_SCREEN_SIZE	6912

cbuffer gConstants : register(b0)
{
	uint total_x;
	uint total_y;
	float translate_x;
	float translate_y;
	float scale_x;
	float scale_y;
	float scale;
	uint screen_bytes;
};

struct VS_INPUT
{
	int tile_x : TILE_X;
	int tile_y : TILE_Y;
	uint tile_w : TILE_W;
	uint tile_h : TILE_H;
	uint offset_x : OFFSET_X;
	uint offset_y : OFFSET_Y;
	uint screen_index : SCREEN;
	uint vertexid : SV_VertexID;
};

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


VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	float tile_x = input.tile_x * 2.0f / total_x;
	float tile_y = input.tile_y * 2.0f / total_y;
	float tile_width = input.tile_w * 2.0f / total_x;
	float tile_height = input.tile_h * 2.0f / total_y;

	if (input.vertexid & 1)
		output.pos.x = tile_x + tile_width - 1.0f;
	else
		output.pos.x = tile_x - 1.0f;

	if (input.vertexid & 2)
		output.pos.y = 1.0f - (tile_y + tile_height);
	else
		output.pos.y = 1.0f - tile_y;

	float offset_x = translate_x * 2.0 / total_x;
	float offset_y = translate_y * 2.0 / total_y;
	output.pos.xy += float2(1.0f, -1.0f);
	output.pos.xy *= scale;
	output.pos.xy *= float2(scale_x, scale_y);
	output.pos.xy += float2(offset_x, -offset_y);
	output.pos.xy -= float2(1.0f, -1.0f);

	output.pos.zw = float2(0.0, 1.0);
	output.tex = float2(input.vertexid % 2, (input.vertexid % 4) / 2);

	output.width = input.tile_w;
	output.height = input.tile_h;
	output.offset_x = input.offset_x;
	output.offset_y = input.offset_y;
	output.offset = input.screen_index * screen_bytes;

	return output;
}
