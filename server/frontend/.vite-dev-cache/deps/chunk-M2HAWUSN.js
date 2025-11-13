import {
  VSlideGroupSymbol
} from "./chunk-4M4QPNHQ.js";
import {
  makeGroupItemProps,
  useGroupItem
} from "./chunk-TGCHCPFR.js";
import {
  genericComponent
} from "./chunk-I5IG4TPY.js";

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
//# sourceMappingURL=chunk-M2HAWUSN.js.map
