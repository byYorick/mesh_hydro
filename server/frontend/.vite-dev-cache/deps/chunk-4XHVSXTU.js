import {
  VWindow,
  VWindowItem,
  makeVWindowItemProps,
  makeVWindowProps
} from "./chunk-JOP4ETM4.js";
import {
  VSheet,
  makeVSheetProps
} from "./chunk-BJSWHGXW.js";
import {
  VBtn
} from "./chunk-XDRDSLAW.js";
import {
  VAvatar
} from "./chunk-S4W5YL43.js";
import {
  VDivider
} from "./chunk-CG57HRTQ.js";
import {
  VDefaultsProvider
} from "./chunk-7L4ZWQEP.js";
import {
  VIcon
} from "./chunk-KGUXEXV6.js";
import {
  makeGroupItemProps,
  makeGroupProps,
  useGroup,
  useGroupItem
} from "./chunk-Y35KG7NE.js";
import {
  genOverlays
} from "./chunk-AI2K6XWQ.js";
import {
  ripple_default
} from "./chunk-OKFONRWX.js";
import {
  IconValue
} from "./chunk-JBMQ7ZRV.js";
import {
  makeDisplayProps,
  useDisplay
} from "./chunk-YNJSP2GN.js";
import {
  useLocale
} from "./chunk-TICGPC5M.js";
import {
  useProxiedModel
} from "./chunk-76O7CYX2.js";
import {
  createSimpleFunctional,
  genericComponent,
  getPropertyFromItem,
  omit,
  pick,
  propsFactory,
  provideDefaults,
  useRender
} from "./chunk-QPYD2SUY.js";
import {
  Fragment,
  computed,
  createBaseVNode,
  createVNode,
  inject,
  mergeProps,
  normalizeClass,
  toRefs,
  withDirectives
} from "./chunk-ZS4T33C3.js";

// node_modules/vuetify/lib/components/VStepper/VStepper.js
import "/app/node_modules/vuetify/lib/components/VStepper/VStepper.css";

// node_modules/vuetify/lib/components/VStepper/shared.js
var VStepperSymbol = Symbol.for("vuetify:v-stepper");

// node_modules/vuetify/lib/components/VStepper/VStepperActions.js
var makeVStepperActionsProps = propsFactory({
  color: String,
  disabled: {
    type: [Boolean, String],
    default: false
  },
  prevText: {
    type: String,
    default: "$vuetify.stepper.prev"
  },
  nextText: {
    type: String,
    default: "$vuetify.stepper.next"
  }
}, "VStepperActions");
var VStepperActions = genericComponent()({
  name: "VStepperActions",
  props: makeVStepperActionsProps(),
  emits: {
    "click:prev": () => true,
    "click:next": () => true
  },
  setup(props, _ref) {
    let {
      emit,
      slots
    } = _ref;
    const {
      t
    } = useLocale();
    function onClickPrev() {
      emit("click:prev");
    }
    function onClickNext() {
      emit("click:next");
    }
    useRender(() => {
      const prevSlotProps = {
        onClick: onClickPrev
      };
      const nextSlotProps = {
        onClick: onClickNext
      };
      return createBaseVNode("div", {
        "class": "v-stepper-actions"
      }, [createVNode(VDefaultsProvider, {
        "defaults": {
          VBtn: {
            disabled: ["prev", true].includes(props.disabled),
            text: t(props.prevText),
            variant: "text"
          }
        }
      }, {
        default: () => {
          var _a;
          return [((_a = slots.prev) == null ? void 0 : _a.call(slots, {
            props: prevSlotProps
          })) ?? createVNode(VBtn, prevSlotProps, null)];
        }
      }), createVNode(VDefaultsProvider, {
        "defaults": {
          VBtn: {
            color: props.color,
            disabled: ["next", true].includes(props.disabled),
            text: t(props.nextText),
            variant: "tonal"
          }
        }
      }, {
        default: () => {
          var _a;
          return [((_a = slots.next) == null ? void 0 : _a.call(slots, {
            props: nextSlotProps
          })) ?? createVNode(VBtn, nextSlotProps, null)];
        }
      })]);
    });
    return {};
  }
});

// node_modules/vuetify/lib/components/VStepper/VStepperHeader.js
var VStepperHeader = createSimpleFunctional("v-stepper-header");

// node_modules/vuetify/lib/components/VStepper/VStepperItem.js
import "/app/node_modules/vuetify/lib/components/VStepper/VStepperItem.css";
var makeStepperItemProps = propsFactory({
  color: String,
  title: String,
  subtitle: String,
  complete: Boolean,
  completeIcon: {
    type: IconValue,
    default: "$complete"
  },
  editable: Boolean,
  editIcon: {
    type: IconValue,
    default: "$edit"
  },
  error: Boolean,
  errorIcon: {
    type: IconValue,
    default: "$error"
  },
  icon: IconValue,
  ripple: {
    type: [Boolean, Object],
    default: true
  },
  rules: {
    type: Array,
    default: () => []
  }
}, "StepperItem");
var makeVStepperItemProps = propsFactory({
  ...makeStepperItemProps(),
  ...makeGroupItemProps()
}, "VStepperItem");
var VStepperItem = genericComponent()({
  name: "VStepperItem",
  directives: {
    vRipple: ripple_default
  },
  props: makeVStepperItemProps(),
  emits: {
    "group:selected": (val) => true
  },
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    const group = useGroupItem(props, VStepperSymbol, true);
    const step = computed(() => (group == null ? void 0 : group.value.value) ?? props.value);
    const isValid = computed(() => props.rules.every((handler) => handler() === true));
    const isClickable = computed(() => !props.disabled && props.editable);
    const canEdit = computed(() => !props.disabled && props.editable);
    const hasError = computed(() => props.error || !isValid.value);
    const hasCompleted = computed(() => props.complete || props.rules.length > 0 && isValid.value);
    const icon = computed(() => {
      if (hasError.value) return props.errorIcon;
      if (hasCompleted.value) return props.completeIcon;
      if (group.isSelected.value && props.editable) return props.editIcon;
      return props.icon;
    });
    const slotProps = computed(() => ({
      canEdit: canEdit.value,
      hasError: hasError.value,
      hasCompleted: hasCompleted.value,
      title: props.title,
      subtitle: props.subtitle,
      step: step.value,
      value: props.value
    }));
    useRender(() => {
      var _a, _b, _c;
      const hasColor = (!group || group.isSelected.value || hasCompleted.value || canEdit.value) && !hasError.value && !props.disabled;
      const hasTitle = !!(props.title != null || slots.title);
      const hasSubtitle = !!(props.subtitle != null || slots.subtitle);
      function onClick() {
        group == null ? void 0 : group.toggle();
      }
      return withDirectives(createBaseVNode("button", {
        "class": normalizeClass(["v-stepper-item", {
          "v-stepper-item--complete": hasCompleted.value,
          "v-stepper-item--disabled": props.disabled,
          "v-stepper-item--error": hasError.value
        }, group == null ? void 0 : group.selectedClass.value]),
        "disabled": !props.editable,
        "type": "button",
        "onClick": onClick
      }, [isClickable.value && genOverlays(true, "v-stepper-item"), createVNode(VAvatar, {
        "key": "stepper-avatar",
        "class": "v-stepper-item__avatar",
        "color": hasColor ? props.color : void 0,
        "size": 24
      }, {
        default: () => {
          var _a2;
          return [((_a2 = slots.icon) == null ? void 0 : _a2.call(slots, slotProps.value)) ?? (icon.value ? createVNode(VIcon, {
            "icon": icon.value
          }, null) : step.value)];
        }
      }), createBaseVNode("div", {
        "class": "v-stepper-item__content"
      }, [hasTitle && createBaseVNode("div", {
        "key": "title",
        "class": "v-stepper-item__title"
      }, [((_a = slots.title) == null ? void 0 : _a.call(slots, slotProps.value)) ?? props.title]), hasSubtitle && createBaseVNode("div", {
        "key": "subtitle",
        "class": "v-stepper-item__subtitle"
      }, [((_b = slots.subtitle) == null ? void 0 : _b.call(slots, slotProps.value)) ?? props.subtitle]), (_c = slots.default) == null ? void 0 : _c.call(slots, slotProps.value)])]), [[ripple_default, props.editable && props.ripple, null]]);
    });
    return {};
  }
});

// node_modules/vuetify/lib/components/VStepper/VStepperWindow.js
var makeVStepperWindowProps = propsFactory({
  ...omit(makeVWindowProps(), ["continuous", "nextIcon", "prevIcon", "showArrows", "touch", "mandatory"])
}, "VStepperWindow");
var VStepperWindow = genericComponent()({
  name: "VStepperWindow",
  props: makeVStepperWindowProps(),
  emits: {
    "update:modelValue": (v) => true
  },
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    const group = inject(VStepperSymbol, null);
    const _model = useProxiedModel(props, "modelValue");
    const model = computed({
      get() {
        var _a;
        if (_model.value != null || !group) return _model.value;
        return (_a = group.items.value.find((item) => group.selected.value.includes(item.id))) == null ? void 0 : _a.value;
      },
      set(val) {
        _model.value = val;
      }
    });
    useRender(() => {
      const windowProps = VWindow.filterProps(props);
      return createVNode(VWindow, mergeProps({
        "_as": "VStepperWindow"
      }, windowProps, {
        "modelValue": model.value,
        "onUpdate:modelValue": ($event) => model.value = $event,
        "class": ["v-stepper-window", props.class],
        "style": props.style,
        "mandatory": false,
        "touch": false
      }), slots);
    });
    return {};
  }
});

// node_modules/vuetify/lib/components/VStepper/VStepperWindowItem.js
var makeVStepperWindowItemProps = propsFactory({
  ...makeVWindowItemProps()
}, "VStepperWindowItem");
var VStepperWindowItem = genericComponent()({
  name: "VStepperWindowItem",
  props: makeVStepperWindowItemProps(),
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    useRender(() => {
      const windowItemProps = VWindowItem.filterProps(props);
      return createVNode(VWindowItem, mergeProps({
        "_as": "VStepperWindowItem"
      }, windowItemProps, {
        "class": ["v-stepper-window-item", props.class],
        "style": props.style
      }), slots);
    });
    return {};
  }
});

// node_modules/vuetify/lib/components/VStepper/VStepper.js
var makeStepperProps = propsFactory({
  altLabels: Boolean,
  bgColor: String,
  completeIcon: IconValue,
  editIcon: IconValue,
  editable: Boolean,
  errorIcon: IconValue,
  hideActions: Boolean,
  items: {
    type: Array,
    default: () => []
  },
  itemTitle: {
    type: String,
    default: "title"
  },
  itemValue: {
    type: String,
    default: "value"
  },
  nonLinear: Boolean,
  flat: Boolean,
  ...makeDisplayProps()
}, "Stepper");
var makeVStepperProps = propsFactory({
  ...makeStepperProps(),
  ...makeGroupProps({
    mandatory: "force",
    selectedClass: "v-stepper-item--selected"
  }),
  ...makeVSheetProps(),
  ...pick(makeVStepperActionsProps(), ["prevText", "nextText"])
}, "VStepper");
var VStepper = genericComponent()({
  name: "VStepper",
  props: makeVStepperProps(),
  emits: {
    "update:modelValue": (v) => true
  },
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    const {
      items: _items,
      next,
      prev,
      selected
    } = useGroup(props, VStepperSymbol);
    const {
      displayClasses,
      mobile
    } = useDisplay(props);
    const {
      completeIcon,
      editIcon,
      errorIcon,
      color,
      editable,
      prevText,
      nextText
    } = toRefs(props);
    const items = computed(() => props.items.map((item, index) => {
      const title = getPropertyFromItem(item, props.itemTitle, item);
      const value = getPropertyFromItem(item, props.itemValue, index + 1);
      return {
        title,
        value,
        raw: item
      };
    }));
    const activeIndex = computed(() => {
      return _items.value.findIndex((item) => selected.value.includes(item.id));
    });
    const disabled = computed(() => {
      if (props.disabled) return props.disabled;
      if (activeIndex.value === 0) return "prev";
      if (activeIndex.value === _items.value.length - 1) return "next";
      return false;
    });
    provideDefaults({
      VStepperItem: {
        editable,
        errorIcon,
        completeIcon,
        editIcon,
        prevText,
        nextText
      },
      VStepperActions: {
        color,
        disabled,
        prevText,
        nextText
      }
    });
    useRender(() => {
      const sheetProps = VSheet.filterProps(props);
      const hasHeader = !!(slots.header || props.items.length);
      const hasWindow = props.items.length > 0;
      const hasActions = !props.hideActions && !!(hasWindow || slots.actions);
      return createVNode(VSheet, mergeProps(sheetProps, {
        "color": props.bgColor,
        "class": ["v-stepper", {
          "v-stepper--alt-labels": props.altLabels,
          "v-stepper--flat": props.flat,
          "v-stepper--non-linear": props.nonLinear,
          "v-stepper--mobile": mobile.value
        }, displayClasses.value, props.class],
        "style": props.style
      }), {
        default: () => {
          var _a, _b;
          return [hasHeader && createVNode(VStepperHeader, {
            "key": "stepper-header"
          }, {
            default: () => [items.value.map((_ref2, index) => {
              let {
                raw,
                ...item
              } = _ref2;
              return createBaseVNode(Fragment, null, [!!index && createVNode(VDivider, null, null), createVNode(VStepperItem, item, {
                default: slots[`header-item.${item.value}`] ?? slots.header,
                icon: slots.icon,
                title: slots.title,
                subtitle: slots.subtitle
              })]);
            })]
          }), hasWindow && createVNode(VStepperWindow, {
            "key": "stepper-window"
          }, {
            default: () => [items.value.map((item) => createVNode(VStepperWindowItem, {
              "value": item.value
            }, {
              default: () => {
                var _a2, _b2;
                return ((_a2 = slots[`item.${item.value}`]) == null ? void 0 : _a2.call(slots, item)) ?? ((_b2 = slots.item) == null ? void 0 : _b2.call(slots, item));
              }
            }))]
          }), (_a = slots.default) == null ? void 0 : _a.call(slots, {
            prev,
            next
          }), hasActions && (((_b = slots.actions) == null ? void 0 : _b.call(slots, {
            next,
            prev
          })) ?? createVNode(VStepperActions, {
            "key": "stepper-actions",
            "onClick:prev": prev,
            "onClick:next": next
          }, slots))];
        }
      });
    });
    return {
      prev,
      next
    };
  }
});

export {
  VStepperActions,
  VStepperHeader,
  VStepperItem,
  VStepperWindow,
  VStepperWindowItem,
  VStepper
};
//# sourceMappingURL=chunk-4XHVSXTU.js.map
