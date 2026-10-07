SPK_SRCS := $(wildcard musicxml/*.musicxml)
SPK_OBJS := $(patsubst musicxml/%.musicxml,spk/%.spk,$(SPK_SRCS))

all: $(SPK_OBJS)

spk/%.spk: musicxml/%.musicxml
	uv run mkspk.py -i $< -o $@

