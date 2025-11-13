import {
  propsFactory
} from "./chunk-I5IG4TPY.js";
import {
  shallowRef,
  toRef,
  watch
} from "./chunk-ZS4T33C3.js";

// node_modules/vuetify/lib/composables/lazy.js
var makeLazyProps = propsFactory({
  eager: Boolean
}, "lazy");
function useLazy(props, active) {
  const isBooted = shallowRef(false);
  const hasContent = toRef(() => isBooted.value || props.eager || active.value);
  watch(active, () => isBooted.value = true);
  function onAfterLeave() {
    if (!props.eager) isBooted.value = false;
  }
  return {
    isBooted,
    hasContent,
    onAfterLeave
  };
}

export {
  makeLazyProps,
  useLazy
};
//# sourceMappingURL=chunk-TEKGGN5Z.js.map
