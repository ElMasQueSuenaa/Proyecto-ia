# ===========================
# Makefile - HC con logs automáticos (robusto)
# ===========================

SHELL := /bin/bash
.ONESHELL:

CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall

SRCS := stroke.cpp testCall.cpp
OBJS := $(SRCS:.cpp=.o)
TARGET := paint

LOGDIR := logs
OUTDIR := outputs

# Crear carpetas si no existen
$(shell mkdir -p $(LOGDIR))
$(shell mkdir -p $(OUTDIR))

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "🔧 Enlazando $(TARGET)..."
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp stroke.h
	@echo "🧩 Compilando $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# RUN_OPTS: target [T] [iters] [seed] [K]
# - Sin target: no se pasan args → el main usa su default y escribe log.txt; lo renombramos.
# - Con target: se pasan todos los args y el logfile como 6º argumento.
run: $(TARGET)
	@echo "🚀 Ejecutando el programa..."
	# parse RUN_OPTS con defaults
	if [[ -n "$(RUN_OPTS)" ]]; then
		set -- $(RUN_OPTS)
	else
		set --
	fi
	target="${1:-}"
	T="${2:-340}"
	iters="${3:-10000}"
	seed="${4:-12345}"
	K="${5:-32}"

	if [[ -n "$$target" ]]; then
		base="$$(basename "$$target")"; base="$${base%.*}"
		timestamp="$$(date +%Y-%m-%d_%H-%M-%S)"
		logfile="$(LOGDIR)/$${base}_$${timestamp}.txt"
		echo "📝 Guardando log en $$logfile"
		./$(TARGET) "$$target" "$$T" "$$iters" "$$seed" "$$K" "$$logfile"
	else
		# sin argumentos → el main usa targetPath por defecto y logFile="log.txt"
		./$(TARGET)
		base="default"
		timestamp="$$(date +%Y-%m-%d_%H-%M-%S)"
		if [[ -f "log.txt" ]]; then
			mv "log.txt" "$(LOGDIR)/$${base}_$${timestamp}.txt"
			echo "📝 Log movido a $(LOGDIR)/$${base}_$${timestamp}.txt"
		fi
	fi

clean:
	@echo "🧹 Limpiando objetos..."
	rm -f $(OBJS)

deepclean: clean
	@echo "🧼 Limpieza profunda..."
	rm -f $(TARGET) $(OUTDIR)/*.png $(LOGDIR)/*.txt
