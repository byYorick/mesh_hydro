import {
  VSlideGroupSymbol
} from "./chunk-BDPWZXPU.js";
import {
  makeGroupItemProps,
  useGroupItem
} from "./chunk-Y35KG7NE.js";
import {
  genericComponent
} from "./chunk-QPYD2SUY.js";

// node_modules/vuetify/lib/components/VSlideGroup/VSlideGroupItem.js
var VSlideGroupItem = genericComponent()({
  name: "VSlideGroupItem",
  props: makeGroupItemProps(),
  emits: {
    "group:selected": (val) => true
  },
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    const slideGroupItem = useGroupItem(props, VSlideGroupSymbol);
    return () => {
      var _a;
      return (_a = slots.default) == null ? void 0 : _a.call(slots, {
        isSelected: slideGroupItem.isSelected.value,
        select: slideGroupItem.select,
        toggle: slideGroupItem.toggle,
        selectedClass: slideGroupItem.selectedClass.value
      });
    };
  }
});

export {
  VSlideGroupItem
};
//# sourceMappingURL=chunk-NHWSVF7Q.js.map
