import {
  onMounted,
  readonly,
  shallowRef,
  toRef
} from "./chunk-ZS4T33C3.js";

// node_modules/vuetify/lib/composables/ssrBoot.js
function useSsrBoot() {
  const isBooted = shallowRef(false);
  onMounted(() => {
    window.requestAnimationFrame(() => {
      isBooted.value = true;
    });
  });
  const ssrBootStyles = toRef(() => !isBooted.value ? {
    transition: "none !important"
  } : void 0);
  return {
    ssrBootStyles,
    isBooted: readonly(isBooted)
  };
}

export {
  useSsrBoot
};
//# sourceMappingURL=chunk-4R7HKKEK.js.map
