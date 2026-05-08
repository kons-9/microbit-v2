# μT-Kernel 3

micro:bit v2.2 (nRF52833) 用の μT-Kernel 3 RTOS。

## セットアップ

パスワード: https://www.t-engine4u.com/info/mbit/2.html

```bash
cd firmware/kernel
./setup.sh
```

または手動:

```bash
PASSWORD=xxxxx
wget https://www.personal-media.co.jp/book/tw/data/362_mbit_mtk3.zip
unzip -P $PASSWORD 362_mbit_mtk3.zip
```

展開後、`mtkernel_3/` ディレクトリが生成される。
