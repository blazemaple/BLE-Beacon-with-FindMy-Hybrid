# BLE Beacon with FindMy Hybrid (FindX) Project

## 功能描述

1. **混合廣播 iBeacon 與 Find My**
   - 本專案實現了 BLE 混合廣播功能，能夠在 iBeacon 與 Find My 模式之間切換。
   - 根據電池電量動態更新廣播數據，確保設備狀態的即時性。
   - 支援自定義廣播模板，滿足不同應用場景需求。

2. **搭配 pca10056e\s112\ses\Output\Release\Exe 使用**
   - 本專案適用於 Nordic Semiconductor 的 PCA10056e 開發板，並使用 S112 SoftDevice BLE 協議棧。
   - 可直接搭配 `pca10056e\s112\ses\Output\Release\Exe` 目錄下的執行檔進行測試與部署。

## 使用說明

1. 將專案放置於 `nRF5 17.1.0 SDK\example\` 目錄下。
2. 將專案燒錄至 PCA10056e 開發板。
3. 使用相應的 BLE 掃描工具驗證廣播數據。

## 注意事項

- 確保開發板已正確配置電池電壓檢測功能。
- 使用前請確認已安裝對應版本的 S112 SoftDevice。
- 可使用其他開發板但需自行加入並更改為相對應的檔案。