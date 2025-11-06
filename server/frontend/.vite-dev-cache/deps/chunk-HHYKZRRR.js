import {
  getCurrentInstance
} from "./chunk-QPYD2SUY.js";

// node_modules/vuetify/lib/composables/scopeId.js
function useScopeId() {
  const vm = getCurrentInstance("useScopeId");
  const scopeId = vm.vnode.scopeId;
  return {
    scopeId: scopeId ? {
      [scopeId]: ""
    } : void 0
  };
}

export {
  useScopeId
};
//# sourceMappingURL=chunk-HHYKZRRR.js.map
