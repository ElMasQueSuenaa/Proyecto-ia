# ===========================
# Makefile - Pintando con Algoritmos
# ===========================

# Compilador y flags
CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall

# Archivos fuente (puedes agregar más .cpp si creas otros módulos)
SRCS := stroke.cpp testCall.cpp
OBJS := $(SRCS:.cpp=.o)

# Nombre del ejecutable
TARGET := paint

# ===========================
# Reglas
# ===========================

# Regla por defecto (compila el ejecutable)
all: $(TARGET)

# Cómo enlazar los objetos en el ejecutable final
$(TARGET): $(OBJS)
	@echo "🔧 Enlazando $(TARGET)..."
	$(CXX) $(CXXFLAGS) -o $@ $^

# Cómo compilar cada archivo fuente individualmente
%.o: %.cpp stroke.h
	@echo "🧩 Compilando $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Ejecutar el programa (compila si es necesario)
run: $(TARGET)
	@echo "🚀 Ejecutando el programa..."
	./$(TARGET)

# Limpieza de archivos compilados
clean:
	@echo "🧹 Limpiando archivos objeto..."
	rm -f $(OBJS)

# Limpieza total (binarios + PNG generados)
deepclean: clean
	@echo "🧼 Limpieza profunda..."
	rm -f $(TARGET) *.png

# ===========================
# Fin del Makefile
# ===========================
