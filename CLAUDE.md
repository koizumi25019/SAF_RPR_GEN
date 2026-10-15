# 開発エージェント向けの作業指針

- 利用方法・ディレクトリ構成は [README.md](README.md) を参照する。
- 本体の設計とブランチの役割は [docs/architecture.md](docs/architecture.md) を参照する。
- LOCの励起条件・観測点・確率の定義は [docs/transition_fault.md](docs/transition_fault.md) を参照する。
- 検証手順は [docs/testing.md](docs/testing.md)、期待値の条件は [tests/expected/README.md](tests/expected/README.md) を参照する。
- 挙動が変わりうる変更では、ビルドとCTestを実行し、c17aのFDPを既知の期待値と照合する。
- 回帰比較は `net_name,f_type,fdp` を使う。キューブ数はソルバの探索順で変わりうる。
- 期待値を実行結果で自動上書きしない。出力はGit管理対象の `output/{saf,tdf}/<条件>/{fdp,log}/` に保存する。
- 本番バイナリに研究用の計測・独立検証フックを混ぜない。詳細検証は隔離ビルドで行う。
