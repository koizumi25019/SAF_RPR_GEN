# 1. 設定セクション 
TARGET_NAME := my_app # 実行ファイル名
SRCDIRS     := Source Source/Compaction/DoubleDetection Source/CPI Source/CPI/CPI_analyze \
	Source/FDiag Source/FSIM Source/FSIM/LSIM Source/FSIM/PPSFP Source/FSIM/PPSFP/PPSFP_Ndrop \
	Source/FSIM/SPPFP Source/FSIM/SPSFP Source/Initialize Source/Lib Source/Netlist \
	Source/Output Source/Reachable_state Source/RTL_STATE Source/XID/CPI_XID Source/XID/SAF_XID \
	Source/XID/SAF_XID/Diff_path_XID Source/XID/SAF_XID/Missed_XID Source/XID/SAF_XID/XID_PPSFP_FSIM \
	Source/XID/SAF_XID/XID_SPPFP_FSIM Source/XID/TDF_XID Source/XID/TDF_XID/Distribution_XID \
	Source/XID/TDF_XID/XID_PPSFP_FSIM Source/XID/TDF_XID/XID_SPPFP_FSIM # ソースファイルがあるディレクトリ名
INCDIRS     := Source # ヘッダファイルのありか

# コンパイラ設定
CC       := gcc # C++があるときはg++にする
CFLAGS   := -Wall -Wextra -fcommon $(addprefix -I, $(INCDIRS)) # 共通のコンパイルオプション
LDLIBS   := -lm # 数学ライブラリ(pow等)
DEBUG_FLAGS   := -g #デバッグ用の実行ファイルを作成する際のコンパイルオプション
RELEASE_FLAGS := -O2 # 最適化された実行ファイル

# 2. ファイル・ディレクトリパスの生成
SRCS := $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.c)) # c++のときは*.cでなく*.cppにする

# デバッグ用パス
DEBUG_DIR  := Build/Debug
DEBUG_OBJS := $(addprefix $(DEBUG_DIR)/, $(notdir $(SRCS:.c=.o)))
DEBUG_BIN  := $(DEBUG_DIR)/$(TARGET_NAME)

# リリース用パス
RELEASE_DIR  := Build/Release
RELEASE_OBJS := $(addprefix $(RELEASE_DIR)/, $(notdir $(SRCS:.c=.o)))
RELEASE_BIN  := $(RELEASE_DIR)/$(TARGET_NAME)

# 検索パスの設定
VPATH := $(SRCDIRS)

# 3. ルールセクション
.PHONY: all debug release clean

all: debug release

debug: $(DEBUG_BIN)
release: $(RELEASE_BIN)

# デバッグ版のビルド
$(DEBUG_BIN): $(DEBUG_OBJS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $@ $^

$(DEBUG_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c -o $@ $<

# リリース版のビルド
$(RELEASE_BIN): $(RELEASE_OBJS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -o $@ $^ $(LDLIBS)

$(RELEASE_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -c -o $@ $<

clean:
	rm -r $(DEBUG_DIR)/*.o $(DEBUG_BIN) $(RELEASE_DIR)/*.o $(RELEASE_BIN)
