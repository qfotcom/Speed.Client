.pragma library

var navGroupOrder = ["client"];
var navGroups = { "client": "SpeedClient" };

var pages = [
    {
        id: "status",
        name: "状态",
        group: "client",
        icon: "activity",
        component: "StatusPage.qml"
    },
    {
        id: "connection",
        name: "连接",
        group: "client",
        icon: "plug",
        component: "ConnectionPage.qml"
    },
    {
        id: "rest",
        name: "REST",
        group: "client",
        icon: "cloud",
        component: "RestPage.qml"
    },
    {
        id: "legacy",
        name: "推送",
        group: "client",
        icon: "radio",
        component: "LegacyPage.qml"
    }
];
