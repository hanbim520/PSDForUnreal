// 导入 Photoshop UXP API
const { action, app, core } = require("photoshop");
// 在插件加载时，获取对各个 UI 面板的引用
const defaultPanel = document.getElementById("default-panel");
const textPanel = document.getElementById("text-panel");
const texturePanel = document.getElementById("texture-panel");
const { storage } = require("uxp");

// 1. 获取按钮元素的引用
const populateButton = document.getElementById("btnPopulate");

populateButton.addEventListener("click", async () => {
    // 3. 在安全的 executeAsModal 中执行文档修改操作
    await core.executeAsModal(async () => {
        try {
            // 检查是否有打开的文档
            if (app.documents.length === 0) {
                core.showAlert("请先打开一个文档。");
                return;
            }

            const activeLayers = app.activeDocument.activeLayers;

            // 检查是否选中了图层
            if (activeLayers.length === 0) {
                core.showAlert("请至少选择一个图层。");
                return;
            }

            // 4. 遍历所有选中的图层并重命名
            activeLayers.forEach(layer => {
                // 在这里定义你的命名规则
                // 示例：在原图层名前面加上一个前缀
                //layer.name = `[PROCESSED] ${layer.name}`;

                 // 获取图层尺寸
                const bounds = layer.bounds;
                const size = [bounds.width, bounds.height];
                const properties = {
                    size: size
                    // 您可以在这里添加更多想写入的属性, 例如：
                    // opacity: layer.opacity,
                    // blendMode: layer.blendMode
                };
                const baseName = layer.name.split('@')[0];
                let type = (layer.kind === "text") ? "Text" : "Image";

                const propertiesString = JSON.stringify(properties);
                const newName = `${baseName}@${type}:${propertiesString}`;
                layer.name = newName;

                 core.showAlert(`图层名称已成功编码为:\n${newName}`);
            });

        } catch (e) {
            core.showAlert(`操作失败: ${e.message}`);
        }
    }, { "commandName": "Populate and Rename Layers" }); // 这个名字会出现在 Photoshop 的历史记录里
});

/**
 * 根据图层名称更新 UI 的可见性
 * @param {string | null} layerName 当前选中图层的名称
 */
function updateUIVisibility(layerNames) {
    // 1. 首先隐藏所有特殊面板
    textPanel.classList.add("hidden");
    texturePanel.classList.add("hidden");
    defaultPanel.classList.add("hidden");

    // 2. 检查是否有选中的图层
    if (layerNames && layerNames.length > 0) {
        // 我们只关心第一个选中的图层来决定UI显示
        const primaryLayerName = layerNames[0];

        console.log("primaryLayerName:", primaryLayerName);

        // 3. 根据图层名决定显示哪个面板
        if (primaryLayerName.includes("@Texture")) {
            texturePanel.classList.remove("hidden");
             console.log("show Texture");
        } else if (primaryLayerName.includes("@Text")) {
            textPanel.classList.remove("hidden");
             console.log("show Text");
        } else {
            // 如果图层名不匹配任何规则，显示默认面板
            defaultPanel.classList.remove("hidden");
        }
    } else {
        // 如果没有选中图层，也显示默认面板
        defaultPanel.classList.remove("hidden");
    }
}

function parseLayerName(layerNames) {
  // 检查数组是否为空
  if (layerNames.length > 0) {
    // 从数组中取出第一个元素
    const firstLayerName = layerNames[0]; // firstLayerName 的值是 "txtA@Text"

    // 使用 .includes() 方法进行检查
    if (firstLayerName.includes("@Text")) {
      console.log("解析成功：图层名包含 '@Text'。");
      return "text"; // 返回一个标识，用于更新UI
    } 
    else if (firstLayerName.includes("@Texture")) {
      console.log("解析成功：图层名包含 '@Texture'。");
      return "texture"; // 返回一个标识
    }
    else {
      console.log("图层名不包含任何特殊标识。");
      return "default";
    }
  } else {
    // 如果没有选中任何图层
    console.log("没有选中的图层。");
    return "default";
  }
}

// 定义事件处理函数
const handleLayerSelect = (event, descriptor) => {
    // 确保我们只处理 'select' 事件
    if (event === 'select') {
        // 使用 executeAsModal 来确保安全地访问文档状态
        core.executeAsModal(async () => {
            try {
                // 检查是否有打开的文档
                if (app.documents.length > 0) {
                    const activeDocument = app.activeDocument;
                    if (activeDocument) {
                        const selectedLayers = activeDocument.activeLayers;
                        const selectedLayerNames = selectedLayers.map(layer => layer.name);
                        console.log("选中的图层已更新:", selectedLayerNames);
                        
                        updateUIVisibility(selectedLayerNames);
                        // 您可以在这里更新您的插件UI或执行其他逻辑
                        // 例如：document.getElementById("layerName").textContent = selectedLayerNames.join(", ");
                    }
                }
            } catch (e) {
                console.error("处理图层选择事件时出错:", e);
            }
        });
    }
};

// 注册 'select' 事件的监听器
try {
    action.addNotificationListener([
        { event: "select" }
    ], handleLayerSelect);
    console.log("已成功添加图层选择事件监听器。");
} catch (e) {
    console.error("添加图层选择事件监听器失败:", e);
}
